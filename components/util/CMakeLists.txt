idf_component_register(
    SRCS "util.c" "mac_handler.c" "chip_id.c" "global_servers.c" "mac_handler.c" "constants.c" "util.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_system esp_wifi esp_http_server types
)
