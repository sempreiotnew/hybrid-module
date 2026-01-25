idf_component_register(
    SRCS "websocket.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server json
)
