#include "HTTPServer.h"

HTTPServer::HTTPServer(std::string_view apiEndpoint) :
        m_APIEndpoint(apiEndpoint) {
    m_APIEndpointSet = m_APIEndpoint + "/set";
    m_APIEndpointGet = m_APIEndpoint + "/get";
}

HTTPServer::~HTTPServer() {
    spdlog::debug("{} Destroying HTTP server", LOG_TAG);
    m_HTTPServer.stop();
    if (m_HTTPS) {
        m_HTTPSServer.stop();
    }
    spdlog::info("{} HTTP server destroyed", LOG_TAG);
}

bool HTTPServer::Init() {
    bool https = m_Config.Get("HTTPS", false);

    m_HTTPServer.config.max_uri_handlers = 100;
    m_HTTPServer.config.stack_size = 8192;

    m_HTTPSServer.config.max_uri_handlers = 100;
    m_HTTPSServer.config.stack_size = 8192;
    m_MainServer = https ? (PsychicHttpServer*) &m_HTTPSServer : &m_HTTPServer;
    bool status = https ? InitHTTPS() : InitHTTP();
    if (!status) {
        return false;
    }

    return true;
}
static std::string CreateLastModifiedHeader(time_t timestamp) {
    struct tm timeinfo;
    gmtime_r(&timestamp, &timeinfo);

    // Day names array
    static const std::array<const char*, 7> dayNames = {
            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    // Month names array
    static const std::array<const char*, 12> monthNames = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    // Format: "Wed, 21 Oct 2015 07:28:00 GMT"
    char buffer[32];
    snprintf(buffer, sizeof(buffer),
             "%s, %02d %s %04d %02d:%02d:%02d GMT",
             dayNames[timeinfo.tm_wday],
             timeinfo.tm_mday,
             monthNames[timeinfo.tm_mon],
             timeinfo.tm_year + 1900,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);

    return std::string(buffer);
}

static std::string GetFileLastModified(const char* path) {
    File file = LittleFS.open(path, "r");
    if(!file) {
        return "";
    }

    time_t lastWrite = file.getLastWrite();
    file.close();

    if(lastWrite == 0) {
        return "";
    }

    return CreateLastModifiedHeader(lastWrite);
}

void HTTPServer::SetFilesToServe(const std::map<std::string, std::string> &files) {
    for (const auto &[uri, file]: files) {
        spdlog::debug("{} Adding file to serve: {} -> {}", LOG_TAG, file, uri);
        std::string lastModified = GetFileLastModified(file.c_str());
        auto handler = new PsychicStaticFileHandler(uri.c_str(), LittleFS, file.c_str(), lastModified.c_str());
        if (uri == "/404") {
            spdlog::debug("{} Adding /404 page as default", LOG_TAG);
            m_MainServer->onNotFound([](PsychicRequest* request) {
                spdlog::info("{} Returning 404 page for {} to client {}",
                             LOG_TAG,
                             request->uri().c_str(),
                             request->client()->remoteIP().toString().c_str());
                return request->redirect("/404");
            });
        }
        m_MainServer->on(uri.c_str(), handler);
    }
}

void HTTPServer::AddEndpoint(std::string_view endpointPath, http_method method, const Handler_t &handlerFunction) {
    spdlog::debug("{} Adding endpoint: {} {}", LOG_TAG, http_method_str(method), endpointPath);
    auto handler = new PsychicWebHandler();
    handler->onRequest(handlerFunction);
    m_MainServer->on(endpointPath.data(), method, handler);
}

void HTTPServer::CreateVariable(
        const std::shared_ptr<std::string> &storage,
        const std::function<bool(std::string_view)> &validator,
        std::string_view paramName,
        bool Get, bool Set
) {
    auto variable = std::make_shared<ServerVariable>(storage, validator, paramName);
    std::string endpoint = m_APIEndpointSet + paramName.data();
    if (Set) AddSetVarEndpoint(endpoint, variable);
    if (Get) {
        endpoint = m_APIEndpointGet + paramName.data();
        AddGetVarEndpoint(endpoint, variable);
    }
}

void HTTPServer::CreateVariable(
        const std::function<std::string()> &getter,
        const std::function<bool(std::string_view)> &setter,
        std::string_view paramName,
        bool Get, bool Set
) {

    auto variable = std::make_shared<ServerVariable>(getter, setter, paramName);
    m_Variables.push_back(variable);

    std::string endpoint = m_APIEndpointSet + paramName.data();
    if (Set) AddSetVarEndpoint(endpoint, variable);
    if (Get) {
        endpoint = m_APIEndpointGet + paramName.data();
        AddGetVarEndpoint(endpoint, variable);
    }
}

void HTTPServer::AddSetVarEndpoint(std::string_view endpointPath, const std::shared_ptr<ServerVariable> &variable) {
    AddEndpoint(endpointPath,
                HTTP_POST,
                [variable](PsychicRequest* request) {
                    spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                                 request->client()->remoteIP().toString().c_str());
                    spdlog::trace("{} Body: {}", LOG_TAG, request->body().c_str());
                    esp_err_t ret;
                    if (!request->hasParam(variable->GetName().data())) {
                        ret = request->reply(404, "text/plain", "NOT_FOUND");
                        if (ret != ESP_OK) {
                            spdlog::error("{} Set failed: could not reply: {}",
                                          LOG_TAG,
                                          esp_err_to_name(ret));
                        }
                        else { spdlog::debug("{} Set failed: missing parameter", LOG_TAG); }
                        return ret;
                    }

                    PsychicWebParameter* param = request->getParam(variable->GetName().data());
                    if (param && !variable->Set(param->value().c_str())) {
                        ret = request->reply(400, "text/plain", "BAD_REQUEST");
                        if (ret != ESP_OK) {
                            spdlog::error("Set failed: could not reply: {}",
                                          LOG_TAG,
                                          esp_err_to_name(ret));
                        }
                        else { spdlog::debug("{} Set failed: invalid value", LOG_TAG); }
                        return ret;
                    }
                    ret = request->reply(200, "text/plain", "OK");
                    if (ret != ESP_OK) {
                        spdlog::error("{} Set failed: could not reply: {}",
                                      LOG_TAG,
                                      esp_err_to_name(ret));
                    }
                    else { spdlog::debug("{} Set success: {} -> {}", LOG_TAG, variable->GetName(), variable->Get()); }
                    return ret;
                });
}

void HTTPServer::AddGetVarEndpoint(std::string_view endpointPath, const std::shared_ptr<ServerVariable> &variable) {
    AddEndpoint(endpointPath,
                HTTP_GET,
                [variable](PsychicRequest* request) {
                    spdlog::info("{} Received {} request from client {}", LOG_TAG, request->uri().c_str(),
                                 request->client()->remoteIP().toString().c_str());
                    std::string varName = variable->GetName().data();
                    std::string value = variable->Get();
                    esp_err_t ret = request->reply(200, "text/plain", value.c_str());
                    if (ret != ESP_OK) {
                        spdlog::error("{} Set failed: could not reply: {}",
                                      LOG_TAG,
                                      esp_err_to_name(ret));
                    }
                    else { spdlog::debug("{} Get success: {} -> {}", LOG_TAG, varName, value); }
                    return ret;
                });
}

void HTTPServer::AddUploadEndpoint(
        std::string_view endpoint,
        const std::function<std::string(std::string_view filename)> &getTargetPath,
        const std::function<void(std::string_view)> &postUpload
) {
    spdlog::debug("{} Adding upload endpoint: {}", LOG_TAG, endpoint);

    auto UploadHandler = new PsychicUploadHandler();
    UploadHandler->onUpload([getTargetPath, postUpload](PsychicRequest* request,
                                                        const String &filename,
                                                        uint64_t index,
                                                        uint8_t* data,
                                                        size_t len,
                                                        bool last) {
        bool* status;
        File file;
        std::string path = getTargetPath(filename.c_str());


        bool isFirst = index == 0;

        if (isFirst) {
            spdlog::info("{} File upload started from client {} to {}",
                         LOG_TAG,
                         request->client()->remoteIP().toString().c_str(),
                         path.c_str());
            file = LittleFS.open(path.c_str(), "w");
            status = new bool(true);
            request->_tempObject = status;
        }
        else {
            status = static_cast<bool*>(request->_tempObject);
            if (!*status) return ESP_FAIL;
            file = LittleFS.open(path.c_str(), "a");
        }

        if (!file) {
            spdlog::error("{} File upload failed: could not open file", LOG_TAG);
            *status = false;
            return ESP_FAIL;
        }

        if (file.write(data, len) != len) {
            spdlog::error("{} File upload failed: could not write to file", LOG_TAG);
            *status = false;
            file.close();
            return ESP_FAIL;
        }

        file.close();
        spdlog::debug("{} File upload: wrote {} bytes to {}", LOG_TAG, len, path.c_str());
        spdlog::info("{} File upload progress: {}%", LOG_TAG, (index + len) * 100 / request->contentLength());

        if (last) {
            spdlog::debug("{} File upload: transfer finished successfully calling postUpload", LOG_TAG);
            postUpload(path);
        }

        return ESP_OK;
    });

    // Called after upload has been handled
    UploadHandler->onRequest([](PsychicRequest* request) {
        bool status = true;
        if (request->_tempObject) { status = *static_cast<bool*>(request->_tempObject); }
        delete static_cast<bool*>(request->_tempObject);
        request->_tempObject = nullptr;

        if (!status) {
            spdlog::error("File upload from client {} to {} failed",
                          LOG_TAG,
                          request->client()->remoteIP().toString().c_str(),
                          request->uri().c_str());
            return request->reply(500, "text/plain", "FAILED");
        }

        spdlog::info("{} File upload from client {} to {} successful",
                     LOG_TAG,
                     request->client()->remoteIP().toString().c_str(),
                     request->uri().c_str());
        return request->reply(200, "text/plain", "OK");
    });

    m_MainServer->on(endpoint.data(), HTTP_POST, UploadHandler);
}

bool HTTPServer::InitHTTP() {
    spdlog::info("Initializing HTTP server");
    int port = m_Config.Get("Port", 80);
    esp_err_t status = m_HTTPServer.listen(port);

    if (status != ESP_OK) {
        spdlog::error("{} Failed to start HTTP server on port {}", LOG_TAG, port);
        return false;
    }
    m_HTTPServer.onNotFound([](PsychicRequest* request) {
        spdlog::info("{} Returning 404 for {} to client {}",
                     LOG_TAG,
                     request->uri().c_str(),
                     request->client()->remoteIP().toString().c_str());
        return request->reply(404, "text/plain", "Not found");
    });

    spdlog::info("{} HTTP server started on port {}", LOG_TAG, port);

    return true;
}

static bool ReadFile(const std::string &path, std::string &content) {
    File file = LittleFS.open(path.c_str(), "r");
    if (!file) {
        spdlog::error("Failed to open file: {}", path);
        return false;
    }

    content.reserve(file.size());
    while (file.available()) {
        content += static_cast<char>(file.read());
    }
    file.close();
    return true;
}

bool HTTPServer::InitHTTPS() {
    spdlog::info("{} Initializing HTTPS server", LOG_TAG);
    std::string key;
    std::string cert;
    if (!ReadFile("/https.key", key) || !ReadFile("/https.crt", cert)) {
        spdlog::error("{} Failed to read key or cert file", LOG_TAG);
        m_Config.Set("HTTPS", false);
        return false;
    }

    int port = m_Config.GetNested("SSL.Port", 443);
    esp_err_t status = m_HTTPSServer.listen(port, key.c_str(), cert.c_str());

    if (status != ESP_OK) {
        spdlog::error("{} Failed to start HTTPS server on port {}", LOG_TAG, port);
        return false;
    }
    m_HTTPSServer.onNotFound([](PsychicRequest* request) {
        spdlog::info("{} Returning 404 for {} to client {}",
                     LOG_TAG,
                     request->uri().c_str(),
                     request->client()->remoteIP().toString().c_str());
        return request->reply(404, "text/plain", "Not found");
    });

    port = m_Config.Get("Port", 80);
    m_HTTPServer.config.ctrl_port = 20420; // just a random port different from the default one
    status = m_HTTPServer.listen(port);
    if (status != ESP_OK) {
        spdlog::error("{} Failed to start HTTP redirect server on port {}", LOG_TAG, port);
        m_HTTPServer.stop();
    }
    else {
        spdlog::info("{} HTTP redirect server started on port {}", LOG_TAG, port);
        m_HTTPServer.onNotFound([](PsychicRequest* request) {
            spdlog::info("{} Received {} request from client {} redirecting to HTTPS",
                         LOG_TAG,
                         request->uri().c_str(),
                         request->client()->remoteIP().toString().c_str());
            std::string url = "https://";
            url += request->host().c_str();
            url += request->url().c_str();
            return request->redirect(url.c_str());
        });
    }

    spdlog::info("{} HTTPS server started on port {}", LOG_TAG, port);
    m_HTTPS = true;
    return true;
}
