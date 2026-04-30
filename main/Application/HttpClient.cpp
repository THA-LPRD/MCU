#include <spdlog/spdlog.h>
#include "SD.h"
#include "HttpClient.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_crt_bundle.h"

static constexpr const char* LOG_TAG = "HttpClient";

HttpClient::HttpClient(int timeout_ms, int max_retries)
    : m_TimeoutMs(timeout_ms)
    , m_MaxRetries(max_retries)
{}

HttpClient::~HttpClient() {
    if (m_Client) {
        esp_http_client_cleanup(m_Client);
        m_Client = nullptr;
    }
}

std::expected<HttpResponse, HttpError> HttpClient::Get(std::string_view url) {
    return PerformWithRetry(url, HTTP_METHOD_GET);
}

std::expected<HttpResponse, HttpError> HttpClient::GetToFile(std::string_view url, std::string_view filepath) {
    std::expected<HttpResponse, HttpError> result = std::unexpected(HttpError{-1, "No attempts made"});

    for (int attempt = 1; attempt <= m_MaxRetries; ++attempt) {
        result = GetToFileImpl(url, filepath);

        if (result.has_value()) return result;

        spdlog::warn("{} GetToFile attempt {}/{} failed: {}", LOG_TAG, attempt, m_MaxRetries, result.error().message);

        if (attempt < m_MaxRetries) vTaskDelay(pdMS_TO_TICKS(500));
    }

    return result;
}

std::expected<HttpResponse, HttpError> HttpClient::GetToFileImpl(std::string_view url, std::string_view filepath) {
    std::string url_str(url);
    std::string filepath_str(filepath);

    if (m_Client) {
        esp_http_client_cleanup(m_Client);
        m_Client = nullptr;
    }

    esp_http_client_config_t config = {
        .url               = url_str.c_str(),
        .method            = HTTP_METHOD_GET,
        .timeout_ms        = m_TimeoutMs,
        .buffer_size       = 2048,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    m_Client = esp_http_client_init(&config);
    if (!m_Client) {
        return std::unexpected(HttpError{-1, "Failed to init HTTP client for " + url_str});
    }

    esp_err_t err = esp_http_client_open(m_Client, 0);
    if (err != ESP_OK) {
        return std::unexpected(HttpError{-1, std::string("Failed to open connection: ") + esp_err_to_name(err)});
    }

    esp_http_client_fetch_headers(m_Client);
    int status_code    = esp_http_client_get_status_code(m_Client);
    int content_length = esp_http_client_get_content_length(m_Client);

    if (status_code != HttpStatus_Ok) {
        esp_http_client_close(m_Client);
        return std::unexpected(HttpError{status_code, "Server returned HTTP " + std::to_string(status_code)});
    }

    File file = SD.open(filepath_str.c_str(), "w");
    if (!file) {
        esp_http_client_close(m_Client);
        return std::unexpected(HttpError{-1, "Failed to open file for writing: " + filepath_str});
    }

    char buffer[2048];
    int  total_read = 0;

    while (true) {
        int read_len = esp_http_client_read(m_Client, buffer, sizeof(buffer));
        if (read_len < 0) {
            file.close();
            esp_http_client_close(m_Client);
            return std::unexpected(HttpError{-1, std::string("Error reading response: ") + esp_err_to_name(read_len)});
        }
        if (read_len == 0) break;

        if (file.write((const uint8_t*)buffer, read_len) != (size_t)read_len) {
            file.close();
            esp_http_client_close(m_Client);
            return std::unexpected(HttpError{-1, "Failed to write chunk to SD card"});
        }

        total_read += read_len;
        if (content_length > 0) {
            spdlog::debug("{} Download progress: {}%", LOG_TAG, (total_read * 100) / content_length);
        }
    }

    file.close();
    esp_http_client_close(m_Client);

    if (total_read == 0) {
        return std::unexpected(HttpError{status_code, "Empty response body"});
    }

    spdlog::info("{} File downloaded successfully ({} bytes)", LOG_TAG, total_read);
    return HttpResponse{status_code, ""};
}

std::expected<HttpResponse, HttpError> HttpClient::Put(std::string_view path, std::string_view body) {
    return PerformWithRetry(path, HTTP_METHOD_PUT, body);
}

std::expected<HttpResponse, HttpError> HttpClient::PerformWithRetry(
        std::string_view path,
        esp_http_client_method_t method,
        std::string_view body)
{
    std::expected<HttpResponse, HttpError> result = std::unexpected(HttpError{-1, "No attempts made"});

    for (int attempt = 1; attempt <= m_MaxRetries; ++attempt) {
        result = Perform(path, method, body);

        if (result.has_value()) {
            return result;
        }

        spdlog::warn("{} Attempt {}/{} failed: {}", LOG_TAG, attempt, m_MaxRetries, result.error().message);

        if (attempt < m_MaxRetries) {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    return result;
}

std::expected<HttpResponse, HttpError> HttpClient::Perform(
        std::string_view path,
        esp_http_client_method_t method,
        std::string_view body)
{
    std::string url_str(path);

    // Cleanup any previous client before reinitializing
    if (m_Client) {
        esp_http_client_cleanup(m_Client);
        m_Client = nullptr;
    }

    esp_http_client_config_t config = {
        .url               = url_str.c_str(),
        .method            = method,
        .timeout_ms        = m_TimeoutMs,
        .buffer_size       = 2048,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    m_Client = esp_http_client_init(&config);
    if (!m_Client) {
        return std::unexpected(HttpError{-1, "Failed to init HTTP client for " + url_str});
    }

    if (!body.empty()) {
        esp_http_client_set_post_field(m_Client, body.data(), body.size());
    }

    esp_err_t err = esp_http_client_open(m_Client, 0);
    if (err != ESP_OK) {
        return std::unexpected(HttpError{-1, std::string("Failed to open connection: ") + esp_err_to_name(err)});
    }

    esp_http_client_fetch_headers(m_Client);
    int status_code = esp_http_client_get_status_code(m_Client);

    std::string response_body = ReadBody();

    esp_http_client_close(m_Client);

    return HttpResponse{status_code, std::move(response_body)};
}

std::string HttpClient::ReadBody() {
    std::string body;
    char        buffer[512];

    while (true) {
        int read_len = esp_http_client_read(m_Client, buffer, sizeof(buffer) - 1);

        if (read_len < 0) {
            spdlog::error("{} Error reading response body: {}", LOG_TAG, esp_err_to_name(read_len));
            break;
        }
        if (read_len == 0) break;

        buffer[read_len] = '\0';
        body += buffer;
    }

    return body;
}