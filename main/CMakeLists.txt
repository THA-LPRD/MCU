set(COMPONENT_REQUIRES
        "arduino-esp32"
        "psychichttp"
        "arduinojson"
        "spdlog"
        "magic_enum"
        "ConfigManager"
        "Drivers"
)

FILE(GLOB_RECURSE SOURCES "*.cpp")

idf_component_register(
        SRCS ${SOURCES}
        EMBED_TXTFILES "config.json"
)

littlefs_create_partition_image(storage ../flash_data FLASH_IN_PROJECT)
