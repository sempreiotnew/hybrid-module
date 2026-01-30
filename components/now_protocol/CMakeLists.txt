idf_component_register(
    SRCS "now_protocol.c" "now_protocol_websocket.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi types util websocket json nvs_handler
)
