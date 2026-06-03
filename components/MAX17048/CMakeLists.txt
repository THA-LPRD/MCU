FILE(GLOB_RECURSE SOURCESCXX "src/*.cpp")

idf_component_register(
        SRCS ${SOURCESCXX}
        INCLUDE_DIRS "src"
        REQUIRES Drivers
)
