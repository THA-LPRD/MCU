FILE(GLOB SOURCES "src/*.cpp")

idf_component_register(SRCS ${SOURCES}
        INCLUDE_DIRS "src"
        REQUIRES spdlog magic_enum ArduinoJson)

