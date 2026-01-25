
#include "esp_http_server.h"
#include "esp_log.h"
#include "global_http_server.h"
#include "global_ws_server.h"
#include "websocket.h"

static const char *TAG = "webserver.c";
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t index_js_start[] asm("_binary_index_js_start");
extern const uint8_t index_js_end[] asm("_binary_index_js_end");

/* ---------- ROOT HTML---------- */
static esp_err_t root_get_handler(httpd_req_t *req) {
  size_t len = index_html_end - index_html_start;
  return httpd_resp_send(req, (const char *)index_html_start, len);
}

/* ---------- ROOT JS---------- */
static esp_err_t js_get_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/javascript");
  size_t len = index_js_end - index_js_start;
  return httpd_resp_send(req, (const char *)index_js_start, len);
}

httpd_handle_t init_web_server() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16;

  httpd_uri_t root = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = root_get_handler,
  };

  httpd_uri_t js = {
      .uri = "/index.js",
      .method = HTTP_GET,
      .handler = js_get_handler,
  };

  httpd_uri_t ws_uri = {.uri = "/ws",
                        .method = HTTP_GET,
                        .handler = ws_handler,
                        .is_websocket = true};

  if (httpd_start(&local_http_server, &config) == ESP_OK) {
    httpd_register_uri_handler(local_http_server, &root);
    httpd_register_uri_handler(local_http_server, &js);
    httpd_register_uri_handler(local_http_server, &ws_uri);

    local_ws_server = local_http_server;

    ESP_LOGI(TAG, "Webserver started");
  }
  return local_http_server;
}
