idf_component_register()

include(FetchContent)

FetchContent_Declare(
        arduinojson
        GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
        GIT_TAG        c1a507c158810f28a8bd34dc0c16fa1b3636add1
        GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(arduinojson)
if(NOT arduinojson_POPULATED)
    FetchContent_Populate(arduinojson)
endif()

target_include_directories(${COMPONENT_LIB} INTERFACE ${arduinojson_SOURCE_DIR}/src)
