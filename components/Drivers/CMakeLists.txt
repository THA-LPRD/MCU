file(GLOB_RECURSE SOURCES "*.cpp")

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "include"
        REQUIRES spdlog esp_event esp_wifi esp_netif driver wpa_supplicant
)
