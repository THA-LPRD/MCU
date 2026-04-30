idf_component_register(REQUIRES arduino-esp32)

include(FetchContent)

FetchContent_Declare(
        urlencode
        GIT_REPOSITORY https://github.com/plageoj/urlencode.git
        GIT_TAG        470562dfcf5acaa08018d951df812205a842d294
        GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(urlencode)
if(NOT urlencode_POPULATED)
    FetchContent_Populate(urlencode)
endif()

target_sources(${COMPONENT_LIB} INTERFACE ${urlencode_SOURCE_DIR}/src/UrlEncode.cpp)
target_include_directories(${COMPONENT_LIB} INTERFACE ${urlencode_SOURCE_DIR}/src)