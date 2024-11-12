idf_component_register()

set(CMAKE_THREAD_LIBS_INIT idf::pthread)

include(FetchContent)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        27cb4c76708608465c413f6d0e6b8d99a4d84302
        GIT_PROGRESS   TRUE
        GIT_SHALLOW    TRUE
)


FetchContent_GetProperties(spdlog)
if (NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()


target_link_libraries(${COMPONENT_LIB} INTERFACE spdlog::spdlog)
