//
// Created by emirhan on 06/11/24.
//
/* */
#include "AppServer.h"
#include <fstream>
#include "esp_http_client.h"
#include "esp_log.h"
#include "SD.h"
#include "WiFiEAP.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "esp_netif.h"


AppServer::~AppServer() {
    spdlog::info("{} Destroyed server application", LOG_TAG);
}

bool AppServer::InitImpl() {
    spdlog::info("{} Initializing server application", LOG_TAG);
    m_WiFi.ConfigureSNTP();

    switch (m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.Auth_Mode", "PSK"))
    {
        case "PSK":
            if (!m_WiFi.Connect(WiFi::Mode::Station,
                                m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.SSID", "your-SSID"),
                                m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.Password", "your-Password"))) {
                return false;
            }
            m_IP = m_WiFi.GetIP(WiFi::Mode::Station);
            break;

        case "EAP":
            if (!m_WiFi.Connect(WiFi::Mode::EAP, 
                                m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.SSID", "your-ssid"), 
                                m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.Password", "your-Password"), 
                                m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.EAP_ID", "your-identity"),
                                m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.EAP_Username", "your-username"),
                                m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.EAP_Cert", "your-certificate"),
                                5)) {
                return false;
            }
            m_IP = m_WiFi.GetIP(WiFi::Mode::EAP);
            break;

        default:
            spdlog::info("{} Unkown WiFi Auth Mode: {}", LOG_TAG, m_ConfigApplication.GetNested<std::string_view>("AppServer.WiFi.Auth_Mode"));
            return false;
            break;
    }
    
    spdlog::error("{} Server application initialized", LOG_TAG);
    return true;
}

bool AppServer::CheckIfRegistered(uint8_t* mac) {
    char checkRegisteredURL[100];

    sprintf(checkRegisteredURL, "%s/api/v1/displays/%02X%02X%02X%02X%02X%02X",
    //lprd.informatik.tha.de
        m_ConfigApplication.GetNested<std::string_view>("AppServer.ServerURL", "http://lprd.informatik.tha.de:3000").data(),
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_http_client_config_t config = {
        .url = checkRegisteredURL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        spdlog::error("{} Failed to initialize HTTP client", LOG_TAG);
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        spdlog::error("{} HTTP GET request failed: {}", LOG_TAG, esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (status_code == HttpStatus_Ok) {
        spdlog::info("{} Display is already registered on server", LOG_TAG);
        return true;
    }
    else if (status_code == HttpStatus_NotFound) {
        spdlog::info("{} Display not registered on server", LOG_TAG);
        return false;
    }
    else {
        spdlog::error("{} Failed to check if display is registered, HTTP code:  {}", LOG_TAG, status_code);
        return false;
    }
}

bool AppServer::RegisterOnServer(uint8_t* mac) {
    spdlog::debug("{} Registering as new display on server", LOG_TAG);

    // Prepare URL
    char registerURL[255];
    sprintf(registerURL, "%s/api/v1/displays/register/%02X%02X%02X%02X%02X%02X",
        m_ConfigApplication.GetNested<std::string_view>("AppServer.ServerURL", "http://lprd.informatik.tha.de:3000").data(),
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Prepare JSON payload
    DynamicJsonDocument newDisplayPayload(1024);
    char friendlyName[100];
    sprintf(friendlyName, "Display %02X%02X%02X%02X%02X%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    newDisplayPayload["friendly_name"] = friendlyName;
    newDisplayPayload["width"] = 800;// EPDL::GetWidth();
    newDisplayPayload["height"] = 480;// EPDL::GetHeight();

    String newDisplayPayloadString;
    serializeJson(newDisplayPayload, newDisplayPayloadString);

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = registerURL,
        .method = HTTP_METHOD_PUT,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        spdlog::error("{} Failed to initialize HTTP client URL: {} JSON: {}", LOG_TAG, registerURL, newDisplayPayloadString.c_str());
        return false;
    }

    // Set headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Content-Length", String(newDisplayPayloadString.length()).c_str());

    // Set post data
    esp_http_client_set_post_field(client, newDisplayPayloadString.c_str(), newDisplayPayloadString.length());

    // Perform the request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        spdlog::error("{} HTTP PUT request failed:  {}", LOG_TAG, esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (status_code == HttpStatus_Ok) {
        spdlog::info("{} Registered as new display on server", LOG_TAG);
        return true;
    }
    else {
        spdlog::error("{} Failed to register as new display, HTTP code: ", LOG_TAG, status_code);
        return false;
    }
}

String AppServer::FetchConfig(uint8_t* mac) {
    spdlog::debug("{} Fetching config from server", LOG_TAG);
    String configPayloadString = "";

    // Prepare URL
    char configURL[255];
    sprintf(configURL, "%s/api/v1/displays/config/%02X%02X%02X%02X%02X%02X", 
            m_ConfigApplication.GetNested<std::string_view>("AppServer.ServerURL", "http://lprd.informatik.tha.de:3000").data(), 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    spdlog::debug("{} Using URL: {}", LOG_TAG, configURL);

    esp_http_client_config_t config = {
        .url = configURL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
        .buffer_size = 2048,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        spdlog::error("{} Failed to initialize HTTP client", LOG_TAG);
        return configPayloadString;
    }

    // Setze Header
    esp_http_client_set_header(client, "Accept", "application/json");

    // Öffne die Verbindung
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        spdlog::error("{} Failed to open HTTP connection: {}", LOG_TAG, esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return configPayloadString;
    }

    // Sende den Request
    esp_http_client_fetch_headers(client);
    
    // Hole den HTTP Status
    int status_code = esp_http_client_get_status_code(client);
    int content_length = esp_http_client_get_content_length(client);
    
    spdlog::debug("{} Status Code: {}, Content Length: {}", LOG_TAG, status_code, content_length);

    if (status_code == HttpStatus_Ok) {
        // Lese die Daten
        std::string response;
        char buffer[512];
        int total_read = 0;
        
        while (true) {
            int read_len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
            spdlog::debug("{} Read chunk of {} bytes", LOG_TAG, read_len);
            
            if (read_len <= 0) {
                break;
            }
            
            buffer[read_len] = 0; // Null-terminieren
            response += buffer;
            total_read += read_len;
        }
        
        spdlog::debug("{} Total bytes read: {}", LOG_TAG, total_read);
        
        if (total_read > 0) {
            configPayloadString = String(response.c_str());
            spdlog::info("{} Config fetched successfully");
            spdlog::debug("{} Config content: {}", LOG_TAG, configPayloadString.c_str());
        } else {
            spdlog::error("{} No data received in response", LOG_TAG);
        }
    } else {
        spdlog::error("{} HTTP request failed with status: {}", LOG_TAG, status_code);
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return configPayloadString;
}

bool AppServer::FetchImg(const std::string& imageURLPath) {
    spdlog::debug("{} Fetching image from server", LOG_TAG);

    // Prepare URL
    char imageURL[255];
    sprintf(imageURL, "%s%s",
        m_ConfigApplication.GetNested<std::string_view>("AppServer.ServerURL", "http://lprd.informatik.tha.de:3000").data(),
        imageURLPath.c_str());
    
    spdlog::debug("{} Using URL: {}", LOG_TAG, imageURL);

    esp_http_client_config_t config = {
        .url = imageURL,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
        .buffer_size = 2048  // Größerer Buffer für Bilder
    };

    std::string path = "/img.png";
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        spdlog::error("{} Failed to initialize HTTP client", LOG_TAG);
        return false;
    }

    // Öffne die Verbindung
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        spdlog::error("{} Failed to open HTTP connection: {}", LOG_TAG, esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    // Sende den Request und hole Header
    esp_http_client_fetch_headers(client);
    
    // Hole den HTTP Status
    int status_code = esp_http_client_get_status_code(client);
    int content_length = esp_http_client_get_content_length(client);
    
    spdlog::debug("{} Status Code: {}, Content Length: {}", LOG_TAG, status_code, content_length);

    if (status_code == HttpStatus_Ok) {
        // Open file on SD card
        File file = SD.open(path.c_str(), "w");
        if (!file) {
            spdlog::error("{} Failed to open file for writing on SD card", LOG_TAG);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return false;
        }

        // Lese und schreibe die Daten
        char buffer[2048];
        int total_read = 0;
        bool success = true;

        while (true) {
            int read_len = esp_http_client_read(client, buffer, sizeof(buffer));
            spdlog::debug("{} Read chunk of {} bytes", LOG_TAG, read_len);

            if (read_len < 0) {
                spdlog::error("{} Error reading data: {}", LOG_TAG, esp_err_to_name(read_len));
                success = false;
                break;
            }

            if (read_len == 0) {
                // Übertragung abgeschlossen
                break;
            }

            if (file.write((const uint8_t*)buffer, read_len) != read_len) {
                spdlog::error("{} Failed to write chunk to SD card", LOG_TAG);
                success = false;
                break;
            }

            total_read += read_len;
            if (content_length > 0) {
                spdlog::info("{} Download progress: {}%", LOG_TAG, (total_read * 100) / content_length);
            }
        }

        file.close();

        if (success && total_read > 0) {
            spdlog::info("{} Image downloaded successfully, total bytes: {}", LOG_TAG, total_read);
            // Validiere die Datei
            File check = SD.open(path.c_str(), "r");
            if (check) {
                size_t fileSize = check.size();
                check.close();
                spdlog::debug("{} Saved file size: {} bytes", LOG_TAG, fileSize);
            }
        } else {
            spdlog::error("{} Failed to download image", LOG_TAG);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return false;
        }
    } else {
        spdlog::error("{} HTTP request failed with status: {}", LOG_TAG, status_code);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return false;
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return true;
}

bool AppServer::DrawImg() {
    spdlog::info("{} Processing image", LOG_TAG);
    int handle = m_Display->CreateImage("/img.png");
    spdlog::debug("{} Got handle: %d", LOG_TAG, handle);
    this->m_Display->BeginFrame();
    this->m_Display->DrawImage(handle, 0, 0);
    this->m_Display->SwapBuffers();
    this->m_Display->EndFrame();
    this->m_Display->DeleteImage(handle);

    return true;
}

uint64_t AppServer::Run() {
    spdlog::info("{} Running Server application", LOG_TAG);

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);

    if (CheckIfRegistered(mac)) {
        spdlog::info("{} Display is already registered on server", LOG_TAG);

        String configPayloadString = FetchConfig(mac);

        spdlog::debug("{} Got following Config {}", LOG_TAG, configPayloadString.c_str());

        DynamicJsonDocument configPayload(1024);
        DeserializationError error = deserializeJson(configPayload, configPayloadString);

        if (error) {
            m_SleepTime = UINT64_MAX; // Sleep endless to preserver battery
            spdlog::error("{} Failed to parse JSON config: {}", LOG_TAG, error.c_str());
        }
        else {
            // Konfigurationswerte setzen
            m_SleepTime = configPayload["valid_for"].as<int>();
            if (m_SleepTime < 1000 && m_SleepTime > 3153600000000000)
            {
                // Mehr als 10 Jahre oder Weniger als 1 Sekunde -> Kein Timer Wakeup
                m_SleepTime = UINT64_MAX;
            }
            
            String imageURLPath = configPayload["file_path"].as<String>();

            // Bild herunterladen und anzeigen
            if (FetchImg(std::string(imageURLPath.c_str()))) {
                DrawImg();
            }
            else {
                spdlog::error("{} Failed to get image data", LOG_TAG);
            }
        }
    }
    else {
        spdlog::info("{} Display is not yet registered on server", LOG_TAG);
        RegisterOnServer(mac);
        m_SleepTime = UINT64_MAX;
    }

    m_Running = false;

    vTaskDelay(3000 / portTICK_PERIOD_MS);
    return m_SleepTime;
}