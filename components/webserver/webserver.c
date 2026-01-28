
#include "constants.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "global_http_server.h"
#include "global_ws_server.h"
#include "now_protocol.h"
#include "now_protocol_t.h"
#include "webserver_json.h"
#include "websocket.h"

static const char *TAG = "webserver.c";
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t index_js_start[] asm("_binary_index_js_start");
extern const uint8_t index_js_end[] asm("_binary_index_js_end");
extern const uint8_t now_js_start[] asm("_binary_now_js_start");
extern const uint8_t now_js_end[] asm("_binary_now_js_end");

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

/* ---------- NOW JS---------- */
static esp_err_t now_js_get_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/javascript");
  size_t len = now_js_end - now_js_start;
  return httpd_resp_send(req, (const char *)now_js_start, len);
}

/* ---------- NOW PAIR---------- */
static esp_err_t pair_post_handler(httpd_req_t *req) {
  char content[256] = {0};

  int received = httpd_req_recv(req, content, sizeof(content) - 1);
  if (received <= 0)
    return ESP_FAIL;

  uint8_t mac[6];
  if (!parse_pair_post_content(content, mac)) {
    return ESP_FAIL;
  }

  send_pair_request(mac, 10, false);

  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

#include <device_now_info.h>
#include <now_protocol_websocket.h>
void mark_device_paired2(const uint8_t *mac, bool paired) {
  for (int i = 0; i < device_count; i++) {
    if (memcmp(devices[i].device_data.mac, mac, 6) == 0) {
      devices[i].paired =
          paired; // <-- add `bool paired;` to your device struct
      char mac_str[18];
      snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      send_to_websocket(devices, "now_nearby_devices_info");

      ESP_LOGI(TAG, "Device marked as paired (runtime only): %s", mac_str);
      return;
    }
  }

  // Device not found, optionally log
  ESP_LOGW(TAG, "Cannot mark device paired: not found in table");
}
/* ---------- NOW UNPAIR---------- */
static esp_err_t pair_delete_handler(httpd_req_t *req) {
  char content[256] = {0};

  int received = httpd_req_recv(req, content, sizeof(content) - 1);
  if (received <= 0)
    return ESP_FAIL;

  uint8_t mac[6];
  if (!parse_pair_post_content(content, mac)) {
    // TODO retornar erro
    return ESP_FAIL;
  }

  delete_peer_by_mac(mac);
  mark_device_paired2(mac, false);

  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

httpd_handle_t init_web_server() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config.max_uri_handlers = 16;
  config.max_open_sockets = 4;
  config.lru_purge_enable = true;

  config.stack_size = 10240;
  config.recv_wait_timeout = 10;
  config.send_wait_timeout = 10;

  config.stack_size = 10240;
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

  httpd_uri_t now_js = {
      .uri = "/now.js",
      .method = HTTP_GET,
      .handler = now_js_get_handler,
  };

  httpd_uri_t pair = {
      .uri = "/api/pair",
      .method = HTTP_POST,
      .handler = pair_post_handler,
  };

  httpd_uri_t unpair = {
      .uri = "/api/pair",
      .method = HTTP_DELETE,
      .handler = pair_delete_handler,
  };

  // httpd_uri_t ws_uri = {.uri = "/ws",
  //                       .method = HTTP_GET,
  //                       .handler = ws_handler,
  //                       .is_websocket = true};
  httpd_uri_t ws_uri = {
      .uri = "/ws",
      .method = HTTP_GET,
      .handler = ws_handler,
      .user_ctx = NULL,
      .is_websocket = true,
      .handle_ws_control_frames = true,
  };

  if (httpd_start(&local_http_server, &config) == ESP_OK) {
    httpd_register_uri_handler(local_http_server, &root);
    httpd_register_uri_handler(local_http_server, &js);
    httpd_register_uri_handler(local_http_server, &now_js);
    httpd_register_uri_handler(local_http_server, &ws_uri);
    httpd_register_uri_handler(local_http_server, &pair);
    httpd_register_uri_handler(local_http_server, &unpair);

    local_ws_server = local_http_server;

    ESP_LOGI(TAG, "Webserver started");
  }
  return local_http_server;
}
