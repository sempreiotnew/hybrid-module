idf_component_register(
    SRCS "webserver.c" "webserver_json.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server esp_wifi json websocket now_protocol
    EMBED_FILES 
        "html/index.html"
        "html/index.js"
        "html/now.js"
)
