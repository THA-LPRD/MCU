#include "AppServer.h"
#include "HttpClient.h"
#include "SD.h"

static constexpr uint64_t kSleepFallback5Min = 5ULL * 60 * 1000 * 1000;
static constexpr uint64_t kSleepFallback24h = 86400000000ULL;

AppServer::~AppServer() {
    spdlog::info("{} Destroyed server application", LOG_TAG);
}

bool AppServer::InitImpl() {
    spdlog::info("{} Initializing server application", LOG_TAG);
    m_WiFi.ConfigureSNTP();

    std::string authMode(m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.Auth_Mode", "PSK"));

    if (authMode == "PSK") {
        if (!m_WiFi.Connect(WiFi::Mode::Station,
                            m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.SSID", "your-SSID"),
                            m_ConfigApplication.GetNested<std::string_view>(
                                "AppServer.WiFi.Password", "your-Password"))) { return false; }
        m_IP = m_WiFi.GetIP(WiFi::Mode::Station);
    }
    else if (authMode == "EAP") {
        if (!m_WiFi.Connect(WiFi::Mode::EAP,
                            m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.SSID", "your-ssid"),
                            m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.Password", "your-Password"),
                            m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.EAP_ID", "your-identity"),
                            m_ConfigApplication.GetNested<std::string_view>(
                                "AppServer.WiFi.EAP_Username", "your-username"),
                            m_ConfigApplication.GetNested<std::string_view>(
                                "AppServer.WiFi.EAP_Cert", "your-certificate"),
                            5)) { return false; }
        m_IP = m_WiFi.GetIP(WiFi::Mode::EAP);
    }
    else {
        spdlog::error("{} Unknown WiFi Auth Mode: {}", LOG_TAG, authMode);
        return false;
    }

    spdlog::info("{} Server application initialized", LOG_TAG);
    return true;
}

std::string AppServer::MacToHex(uint8_t* mac) {
    char buf[13];
    sprintf(buf, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

std::string AppServer::ServerURL() {
    return std::string(m_ConfigApplication.GetNested<std::string_view>(
        "AppServer.ServerURL", "http://lprd.informatik.tha.de:3000"));
}

bool AppServer::CheckIfRegistered(uint8_t* mac) {
    HttpClient http;
    auto result = http.Get(ServerURL() + "/api/v1/displays/" + MacToHex(mac));

    if (!result) {
        spdlog::error("{} CheckIfRegistered failed: {}", LOG_TAG, result.error().message);
        return false;
    }

    if (result->status_code == HttpStatus_Ok) {
        spdlog::info("{} Display is already registered on server", LOG_TAG);
        return true;
    }
    else if (result->status_code == HttpStatus_NotFound) {
        spdlog::info("{} Display not registered on server", LOG_TAG);
        return false;
    }
    else {
        spdlog::error("{} Failed to check registration, HTTP code: {}", LOG_TAG, result->status_code);
        return false;
    }
}

bool AppServer::RegisterOnServer(uint8_t* mac) {
    spdlog::debug("{} Registering as new display on server", LOG_TAG);

    std::string macHex = MacToHex(mac);

    JsonDocument payload;
    payload["friendly_name"] = "Display " + macHex;
    payload["width"] = 800; // EPDL::GetWidth();
    payload["height"] = 480; // EPDL::GetHeight();

    String payloadStr;
    serializeJson(payload, payloadStr);

    HttpClient http;
    auto result = http.Put(
        ServerURL() + "/api/v1/displays/register/" + macHex,
        std::string_view(payloadStr.c_str(), payloadStr.length()));

    if (!result) {
        spdlog::error("{} RegisterOnServer failed: {}", LOG_TAG, result.error().message);
        return false;
    }

    if (result->status_code == HttpStatus_Ok) {
        spdlog::info("{} Registered as new display on server", LOG_TAG);
        return true;
    }
    else {
        spdlog::error("{} Failed to register, HTTP code: {}", LOG_TAG, result->status_code);
        return false;
    }
}

std::string AppServer::FetchConfig(uint8_t* mac) {
    spdlog::debug("{} Fetching config from server", LOG_TAG);

    HttpClient http;
    auto result = http.Get(ServerURL() + "/api/v1/displays/config/" + MacToHex(mac));

    if (!result) {
        spdlog::error("{} FetchConfig failed: {}", LOG_TAG, result.error().message);
        return "";
    }

    if (result->status_code != HttpStatus_Ok) {
        spdlog::error("{} FetchConfig failed, HTTP code: {}", LOG_TAG, result->status_code);
        return "";
    }

    spdlog::info("{} Config fetched successfully", LOG_TAG);
    spdlog::debug("{} Config content: {}", LOG_TAG, result->body);
    return result->body;
}

bool AppServer::FetchImg(std::string_view imageURLPath) {
    spdlog::debug("{} Fetching image from server", LOG_TAG);

    std::string imageURL;
    if (imageURLPath.starts_with("http://") || imageURLPath.starts_with("https://")) { imageURL = imageURLPath; }
    else { imageURL = ServerURL() + std::string(imageURLPath); }

    spdlog::debug("{} Using URL: {}", LOG_TAG, imageURL);

    HttpClient http;
    auto result = http.GetToFile(imageURL, "/img.png");

    if (!result) {
        spdlog::error("{} FetchImg failed: {}", LOG_TAG, result.error().message);
        return false;
    }

    spdlog::info("{} Image downloaded successfully", LOG_TAG);
    return true;
}

bool AppServer::DrawImg() {
    spdlog::info("{} Processing image", LOG_TAG);
    int handle = m_Display->CreateImage("/img.png");
    spdlog::debug("{} Got handle: {}", LOG_TAG, handle);
    m_Display->BeginFrame();
    m_Display->DrawImage(handle, 0, 0);
    m_Display->SwapBuffers();
    m_Display->EndFrame();
    m_Display->DeleteImage(handle);
    return true;
}

uint64_t AppServer::Run() {
    spdlog::info("{} Running Server application", LOG_TAG);

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);

    if (!CheckIfRegistered(mac)) {
        spdlog::info("{} Display is not yet registered, registering now", LOG_TAG);
        RegisterOnServer(mac);
        m_Running = false;
        m_SleepTime = kSleepFallback24h;
        return m_SleepTime;
    }

    // --- Fetch config ---
    std::string configStr = FetchConfig(mac);
    if (configStr.empty()) {
        spdlog::error("{} Config fetch failed – sleeping 5 minutes", LOG_TAG);
        m_Running = false;
        m_SleepTime = kSleepFallback5Min;
        return m_SleepTime;
    }

    JsonDocument configPayload;
    DeserializationError error = deserializeJson(configPayload, configStr);
    if (error) {
        spdlog::error("{} Failed to parse config JSON: {} – sleeping 5 minutes", LOG_TAG, error.c_str());
        m_Running = false;
        m_SleepTime = kSleepFallback5Min;
        return m_SleepTime;
    }

    int configTime = configPayload["valid_for"].as<int>();
    if (configTime < 0) {
        m_SleepTime = kSleepFallback24h;
        spdlog::info("{} No valid time in config – sleeping 24 h", LOG_TAG);
    }
    else {
        m_SleepTime = std::min(static_cast<uint64_t>(configTime) * 1000ULL * 1000ULL, kSleepFallback24h);
        spdlog::info("{} Sleeping for {} s ({} µs)", LOG_TAG, configTime, m_SleepTime);
    }

    // --- Fetch image ---
    std::string imageURLPath = configPayload["file_path"].as<std::string>();
    if (!FetchImg(imageURLPath)) {
        spdlog::error("{} Image fetch failed – sleeping 5 minutes", LOG_TAG);
        m_Running = false;
        m_SleepTime = kSleepFallback5Min;
        return m_SleepTime;
    }

    DrawImg();

    m_Running = false;
    vTaskDelay(pdMS_TO_TICKS(3000));
    spdlog::info("{} Return sleep value: {} µs", LOG_TAG, m_SleepTime);
    return m_SleepTime;
}