

#include "esp_http_server.h"
#include "esp_log.h"
#include <global_http_server.h>
#include <global_ws_server.h>
#include <string.h>

static const char *TAG = "websocket.c";
static int g_ws_fd = -1; // client socket fd

esp_err_t ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    // WebSocket handshake
    g_ws_fd = httpd_req_to_sockfd(req);
    local_ws_server = req->handle;

    ESP_LOGI(TAG, "WebSocket client connected (fd=%d)", g_ws_fd);
    return ESP_OK;
  }

  httpd_ws_frame_t frame = {
      .final = true,
      .fragmented = false,
  };

  uint8_t buf[128];
  frame.payload = buf;

  // Receive frame
  esp_err_t ret = httpd_ws_recv_frame(req, &frame, sizeof(buf));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "WS recv failed: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "WS RX: %.*s", frame.len, (char *)frame.payload);

  return ESP_OK;
}
void ws_send_text(const char *msg) {
  if (g_ws_fd < 0 || local_ws_server == NULL)
    return;

  httpd_ws_frame_t frame = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t *)msg,
      .len = strlen(msg),
  };

  esp_err_t err = httpd_ws_send_frame_async(local_ws_server, g_ws_fd, &frame);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "WS send failed: %s", esp_err_to_name(err));
    g_ws_fd = -1;
  }
}