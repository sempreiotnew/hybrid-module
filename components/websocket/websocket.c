#include "esp_http_server.h"
#include "esp_log.h"
#include <global_http_server.h>
#include <global_ws_server.h>
#include <string.h>

static const char *TAG = "websocket.c";
static int g_ws_fd = -1;

/* ---------- FORCE CLOSE ---------- */
static void ws_force_close(int fd) {
  if (local_ws_server && fd >= 0) {
    ESP_LOGI(TAG, "Closing WS fd=%d", fd);
    httpd_sess_trigger_close(local_ws_server, fd);
    if (fd == g_ws_fd)
      g_ws_fd = -1;
  }
}

static inline void ws_kill_session(httpd_req_t *req) {
  int fd = httpd_req_to_sockfd(req);

  ESP_LOGW(TAG, "Killing WS session fd=%d", fd);

  httpd_sess_trigger_close(req->handle, fd);

  if (fd == g_ws_fd)
    g_ws_fd = -1;
}

/* ---------- WS HANDLER ---------- */
esp_err_t ws_handler(httpd_req_t *req) {
  int fd = httpd_req_to_sockfd(req);

  /* Handshake */
  if (req->method == HTTP_GET) {
    g_ws_fd = fd;
    local_ws_server = req->handle;
    ESP_LOGI(TAG, "WS connected fd=%d", fd);
    return ESP_OK;
  }

  if (fd != g_ws_fd) {
    return ESP_OK;
  }

  httpd_ws_frame_t frame = {0};

  /* STEP 1: header only */
  esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
  if (ret != ESP_OK) {
    ws_kill_session(req);
    return ESP_OK; // NEVER FAIL
  }

  if (frame.type == HTTPD_WS_TYPE_CLOSE) {
    ws_kill_session(req);
    return ESP_OK;
  }

  if (frame.len == 0 || frame.len > 127) {
    ws_kill_session(req);
    return ESP_OK;
  }

  uint8_t buf[128];
  frame.payload = buf;

  /* STEP 2: payload */
  ret = httpd_ws_recv_frame(req, &frame, frame.len);
  if (ret != ESP_OK) {
    ws_kill_session(req);
    return ESP_OK;
  }

  buf[frame.len] = 0;
  ESP_LOGI(TAG, "WS RX: %s", buf);

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
      ESP_LOGW(TAG, "WS send failed fd=%d", g_ws_fd);
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
