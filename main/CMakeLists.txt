set(COMPONENT_REQUIRES
        "arduino-esp32"
        "psychichttp"
        "arduinojson"
        "spdlog"
        "magic_enum"
        "ConfigManager"
)

FILE(GLOB_RECURSE SOURCES "*.cpp")

idf_component_register(SRCS ${SOURCES})

littlefs_create_partition_image(storage ../flash_data FLASH_IN_PROJECT)
