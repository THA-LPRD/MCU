#include "HTTPServer.h"

HTTPServer::HTTPServer(const std::shared_ptr<spdlog::logger> &logger, std::string_view apiEndpoint) :
        m_Logger(logger),
        m_APIEndpoint(apiEndpoint) {
    m_APIEndpointSet = m_APIEndpoint + "/set";
    m_APIEndpointGet = m_APIEndpoint + "/get";
    std::unordered_map<Config, std::string> defaultConfig = {
            {Config::HTTPPort,     "80"},
            {Config::HTTPSPort,    "443"},
            {Config::HTTPSKey,     ""},
            {Config::HTTPSCert,    ""},
            {Config::HTTPAuth,     ""},
            {Config::HTTPAuthUser, ""},
            {Config::HTTPAuthPass, ""},
            {Config::HTTPS,        "false"}
    };
    m_ConfigManager = std::make_unique<ConfigManager<Config>>(logger, defaultConfig);
    m_ConfigManager->SetValidator(Config::HTTPPort, [](std::string_view value) {
        int port;
        auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), port);
        if (ec == std::errc{} && port > 0 && port < 65535) {
            return true;
        }
        return false;
    });
    m_ConfigManager->SetValidator(Config::HTTPSPort, [](std::string_view value) {
        int port;
        auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), port);
        if (ec == std::errc{} && port > 0 && port < 65535) {
            return true;
        }
        return false;
    });
    m_ConfigManager->SetValidator(Config::HTTPS, [](std::string_view value) {
        return value == "true" || value == "false";
    });
    m_ConfigManager->SetValidator(Config::HTTPAuth, [](std::string_view value) {
        return value == "true" || value == "false";
    });
    m_ConfigManager->SetValidator(Config::HTTPAuthUser, [](std::string_view value) {
        return !value.empty();
    });
    m_ConfigManager->SetValidator(Config::HTTPAuthPass, [](std::string_view value) {
        return !value.empty();
    });
}

HTTPServer::~HTTPServer() {
    for (auto handler: m_Handlers) {
        delete handler;
    }
}

bool HTTPServer::Init(bool https) {
    Set(Config::HTTPS, https ? "true" : "false");

    m_HTTPServer.config.max_uri_handlers = 50;
    m_HTTPServer.config.stack_size = 8192;

    m_HTTPSServer.config.max_uri_handlers = 50;
    m_HTTPSServer.config.stack_size = 8192;
    m_MainServer = https ? (PsychicHttpServer*) &m_HTTPSServer : &m_HTTPServer;
    bool status = https ? InitHTTPS() : InitHTTP();
    if (!status) {
        return false;
    }

    m_Logger->debug("Adding redirect from / to /index.html");
    m_MainServer->on("/", HTTP_GET, [this](PsychicRequest* request) {
        m_Logger->info("Received {} request from client {} redirecting to /index.html", request->uri().c_str(),
                       request->client()->remoteIP().toString().c_str());
        return request->redirect("/index.html");
    });

    return true;
}

void HTTPServer::SetFilesToServe(const std::map<std::string, std::string> &files) {
    for (const auto &[uri, file]: files) {
        m_Logger->debug("Adding file to serve: {} -> {}", file, uri);
        auto handler = new PsychicStaticFileHandler(uri.c_str(), LittleFS, file.c_str(), nullptr);
        m_MainServer->addHandler(handler);
        m_Handlers.push_back(handler);
    }
}

void HTTPServer::AddEndpoint(std::string_view endpointPath, http_method method, const Handler_t &handlerFunction) {
    m_Logger->debug("Adding endpoint: {} {}", http_method_str(method), endpointPath);
    auto handler = new PsychicWebHandler();
    handler->onRequest(handlerFunction);
    m_MainServer->on(endpointPath.data(), method, handler);
    m_Handlers.push_back(handler);
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
    AddSetVarEndpoint(endpoint, variable);
    endpoint = m_APIEndpointGet + paramName.data();
    AddGetVarEndpoint(endpoint, variable);
}

void HTTPServer::AddSetVarEndpoint(std::string_view endpointPath, const std::shared_ptr<ServerVariable> &variable) {
    AddEndpoint(endpointPath,
                HTTP_POST,
                [this, variable](PsychicRequest* request) {
                    m_Logger->info("Received {} request from client {}", request->uri().c_str(),
                                   request->client()->remoteIP().toString().c_str());
                    m_Logger->trace("Body: {}", request->body().c_str());
                    if (!request->hasParam(variable->GetName().data())) {
                        m_Logger->debug("Set failed: missing parameter");
                        return request->reply(404, "text/plain", "NOT_FOUND");
                    }

                    PsychicWebParameter* param = request->getParam(variable->GetName().data());
                    if (param && !variable->Set(param->value().c_str())) {
                        m_Logger->debug("Set failed: invalid value");
                        return request->reply(400, "text/plain", "BAD_REQUEST");
                    }
                    m_Logger->debug("Set success: {} -> {}", variable->GetName(), variable->Get());
                    return request->reply(200, "text/plain",  "OK");
                });
}

void HTTPServer::AddGetVarEndpoint(std::string_view endpointPath, std::shared_ptr<ServerVariable> variable) {
    AddEndpoint(endpointPath,
                HTTP_GET,
                [this, variable](PsychicRequest* request) {
                    m_Logger->info("Received {} request from client {}", request->uri().c_str(),
                                   request->client()->remoteIP().toString().c_str());
                    std::string varName = variable->GetName().data();
                    std::string value = variable->Get();
                    m_Logger->debug("Get success: {} -> {}", varName, value);
                    return request->reply(200, "text/plain", value.c_str());
                });
}

void HTTPServer::AddUploadEndpoint(
        std::string_view endpoint,
        const std::function<std::string(std::string_view filename)> &getTargetPath,
        const std::function<void(std::string_view)> &postUpload
) {
    m_Logger->debug("Adding upload endpoint: {}", endpoint);

    auto UploadHandler = new PsychicUploadHandler();
    UploadHandler->onUpload([this, getTargetPath, postUpload](PsychicRequest* request,
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
            m_Logger->info("File upload started from client {} to {}",
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
            m_Logger->error("File upload failed: could not open file");
            *status = false;
            return ESP_FAIL;
        }

        if (file.write(data, len) != len) {
            m_Logger->error("File upload failed: could not write to file");
            *status = false;
            file.close();
            return ESP_FAIL;
        }

        file.close();
        m_Logger->debug("File upload: wrote {} bytes to {}", len, path.c_str());
        m_Logger->info("File upload progress: {}%", (index + len) * 100 / request->contentLength());

        if (last) {
            m_Logger->debug("File upload: transfer finished successfully calling postUpload");
            postUpload(path);
        }

        return ESP_OK;
    });

    // Called after upload has been handled
    UploadHandler->onRequest([this](PsychicRequest* request) {
        bool status = true;
        if (request->_tempObject) { status = *static_cast<bool*>(request->_tempObject); }
        delete static_cast<bool*>(request->_tempObject);
        request->_tempObject = nullptr;

        if (!status) {
            m_Logger->error("File upload from client {} to {} failed",
                            request->client()->remoteIP().toString().c_str(),
                            request->uri().c_str());
            return request->reply(500, "text/plain", "FAILED");
        }

        m_Logger->info("File upload from client {} to {} successful",
                       request->client()->remoteIP().toString().c_str(),
                       request->uri().c_str());
        return request->reply(200, "text/plain", "OK");
    });

    m_MainServer->on(endpoint.data(), HTTP_POST, UploadHandler);
    m_Handlers.push_back(UploadHandler);
}

bool HTTPServer::InitHTTP() {
    m_Logger->info("Initializing HTTP server");
    int port = m_ConfigManager->GetInt(Config::HTTPPort);
    esp_err_t status = m_HTTPServer.listen(port);

    if (status != ESP_OK) {
        m_Logger->error("Failed to start HTTP server on port {}", port);
        return false;
    }
    m_HTTPServer.onNotFound([this](PsychicRequest* request) {
        m_Logger->info("Returning 404 for {} to client {}",
                       request->uri().c_str(),
                       request->client()->remoteIP().toString().c_str());
        return request->reply(404, "text/plain", "Not found");
    });

    m_Logger->info("HTTP server started on port {}", port);

    return true;
}

bool HTTPServer::InitHTTPS() {
    m_Logger->info("Initializing HTTPS server");
    // TODO: retrive key and cert from LittleFS
    std::string key;
    std::string cert;
    int port = m_ConfigManager->GetInt(Config::HTTPSPort);
    esp_err_t status = m_HTTPSServer.listen(port, key.c_str(), cert.c_str());

    if (status != ESP_OK) {
        m_Logger->error("Failed to start HTTPS server on port {}", port);
        return false;
    }
    m_HTTPSServer.onNotFound([this](PsychicRequest* request) {
        m_Logger->info("Returning 404 for {} to client {}",
                       request->uri().c_str(),
                       request->client()->remoteIP().toString().c_str());
        return request->reply(404, "text/plain", "Not found");
    });

    port = m_ConfigManager->GetInt(Config::HTTPPort);
    m_HTTPServer.config.ctrl_port = 20420; // just a random port different from the default one
    status = m_HTTPServer.listen(port);
    if (status != ESP_OK) {
        m_Logger->error("Failed to start HTTP redirect server on port {}", port);
        m_HTTPServer.stop();
    }
    else {
        m_Logger->info("HTTP redirect server started on port {}", port);
        m_HTTPServer.onNotFound([this](PsychicRequest* request) {
            m_Logger->info("Received {} request from client {} redirecting to HTTPS", request->uri().c_str(),
                           request->client()->remoteIP().toString().c_str());
            std::string url = "https://";
            url += request->host().c_str();
            url += request->url().c_str();
            return request->redirect(url.c_str());
        });
    }

    m_Logger->info("HTTPS server started on port {}", port);
    return true;
}
