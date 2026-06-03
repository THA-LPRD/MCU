#ifndef LPRD_MCU_HTTPCLIENT_H
#define LPRD_MCU_HTTPCLIENT_H

#include <expected>
#include <string>
#include <string_view>
#include "esp_http_client.h"

struct HttpError {
    int status_code; // -1 if no response (connection failed etc.)
    std::string message;
};

struct HttpResponse {
    int status_code;
    std::string body;
};

class HttpClient {
public:
    explicit HttpClient(int timeout_ms = 15000, int max_retries = 3);
    ~HttpClient();

    // Non-copyable, movable
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = default;
    HttpClient& operator=(HttpClient&&) = default;

    std::expected<HttpResponse, HttpError> Get(std::string_view url);
    std::expected<HttpResponse, HttpError> GetToFile(std::string_view url, std::string_view filepath);
    std::expected<HttpResponse, HttpError> Put(std::string_view url, std::string_view body = {});
    std::expected<HttpResponse, HttpError> Post(std::string_view url, std::string_view body = {});

private:
    std::expected<HttpResponse, HttpError> Perform(std::string_view url,
                                                   esp_http_client_method_t method,
                                                   std::string_view body = {});

    std::expected<HttpResponse, HttpError> PerformWithRetry(std::string_view url,
                                                            esp_http_client_method_t method,
                                                            std::string_view body = {});

    std::expected<HttpResponse, HttpError> GetToFileImpl(std::string_view url, std::string_view filepath);
    std::string ReadBody();

    int m_TimeoutMs;
    int m_MaxRetries;
    esp_http_client_handle_t m_Client = nullptr;
};

#endif //LPRD_MCU_HTTPCLIENT_H
