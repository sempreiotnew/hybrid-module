

#include "esp_http_server.h"
#include "esp_log.h"
#include <global_http_server.h>
#include <global_ws_server.h>
#include <string.h>

static const char *TAG = "websocket.c";
static int g_ws_fd = -1; // client socket fd

/* ---------- FORCE CLOSE ---------- */
static void ws_force_close(int fd) {
  if (local_ws_server && fd >= 0) {
    ESP_LOGI(TAG, "Closing old WS fd=%d", fd);
    httpd_sess_trigger_close(local_ws_server, fd);
    if (fd == g_ws_fd)
      g_ws_fd = -1;
  }
}

/* ---------- WS HANDLER ---------- */
esp_err_t ws_handler(httpd_req_t *req) {
  int fd = httpd_req_to_sockfd(req);

  /* WebSocket handshake */
  if (req->method == HTTP_GET) {
    /* If there’s already an active client, close it first */
    if (g_ws_fd >= 0 && g_ws_fd != fd) {
      ws_force_close(g_ws_fd);
    }

    g_ws_fd = fd;
    local_ws_server = req->handle;
    ESP_LOGI(TAG, "WS connected fd=%d", fd);
    return ESP_OK;
  }

  /* If no active FD, just ignore */
  if (g_ws_fd < 0)
    return ESP_OK;

  httpd_ws_frame_t frame = {0};
  uint8_t buf[128];
  frame.payload = buf;

  /* Receive frame safely */
  esp_err_t ret = httpd_ws_recv_frame(req, &frame, sizeof(buf));
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "WS recv failed fd=%d: %s", fd, esp_err_to_name(ret));
    if (fd == g_ws_fd) {
      ws_force_close(fd); // close old FD
    }
    return ESP_OK; // do not crash HTTP server
  }

  if (frame.type == HTTPD_WS_TYPE_CLOSE) {
    ESP_LOGI(TAG, "WS closed by client fd=%d", fd);
    if (fd == g_ws_fd)
      ws_force_close(fd);
    return ESP_OK;
  }

  if (frame.len == 0 || frame.len > sizeof(buf) - 1) {
    ESP_LOGW(TAG, "WS invalid length fd=%d len=%d", fd, frame.len);
    if (fd == g_ws_fd)
      ws_force_close(fd);
    return ESP_OK;
  }

  ret = httpd_ws_recv_frame(req, &frame, frame.len);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "WS payload recv failed fd=%d", fd);
    if (fd == g_ws_fd)
      ws_force_close(fd);
    return ESP_OK;
  }

  buf[frame.len] = 0;
  ESP_LOGI(TAG, "WS RX fd=%d: %s", fd, buf);

  return ESP_OK;
}

/* ---------- ASYNC SEND ---------- */
typedef struct {
  char *msg;
} ws_job_t;

static void ws_send_job(void *arg) {
  ws_job_t *job = arg;

  if (g_ws_fd >= 0 && local_ws_server) {
    httpd_ws_frame_t frame = {.final = true,
                              .type = HTTPD_WS_TYPE_TEXT,
                              .payload = (uint8_t *)job->msg,
                              .len = strlen(job->msg)};

    esp_err_t ret = httpd_ws_send_frame_async(local_ws_server, g_ws_fd, &frame);
    if (ret != ESP_OK) {
      ESP_LOGW(TAG, "WS send failed, closing fd=%d", g_ws_fd);
      ws_force_close(g_ws_fd);
    }
  }

  free(job->msg);
  free(job);
}

void ws_send_text(const char *msg) {
  if (!msg || g_ws_fd < 0 || !local_ws_server)
    return;

  ws_job_t *job = calloc(1, sizeof(ws_job_t));
  if (!job)
    return;

  job->msg = strdup(msg);
  if (!job->msg) {
    free(job);
    return;
  }

  httpd_queue_work(local_ws_server, ws_send_job, job);
}