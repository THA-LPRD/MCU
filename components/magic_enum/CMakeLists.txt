idf_component_register()

include(FetchContent)

FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG        dd6a39d0ba1852cf06907e0f0573a2a10d23c2ad
        GIT_PROGRESS   TRUE
        GIT_SHALLOW    TRUE
)

set(MAGIC_ENUM_OPT_BUILD_EXAMPLES OFF)
set(MAGIC_ENUM_OPT_BUILD_TESTS OFF)
set(MAGIC_ENUM_OPT_INSTALL OFF)
set(MAGIC_ENUM_OPT_INSTALL_PACKAGE_XML OFF)
FetchContent_GetProperties(magic_enum)
if (NOT magic_enum_POPULATED)
    FetchContent_Populate(magic_enum)
    add_subdirectory(${magic_enum_SOURCE_DIR} ${magic_enum_BINARY_DIR})
endif()

target_link_libraries(${COMPONENT_LIB} INTERFACE magic_enum::magic_enum)
