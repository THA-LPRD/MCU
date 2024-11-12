FILE(GLOB_RECURSE SOURCESCXX "src/*.cpp")
FILE(GLOB_RECURSE SOURCESC "src/*.c")

idf_component_register(SRCS ${SOURCESCXX} ${SOURCESC}
        INCLUDE_DIRS "src"
        REQUIRES spdlog magic_enum Drivers arduino-esp32 libspng
)

add_compile_definitions(
        SPNG_STATIC
)
