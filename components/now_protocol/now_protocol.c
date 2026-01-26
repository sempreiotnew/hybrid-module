
#include "constants.h"
#include "device_now_info.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "now_protocol_json.h"
#include "now_protocol_t.h"
#include "websocket.h"

static const char *TAG = "now_protocol.c";

void update_or_add_device(const esp_now_recv_info_t *info,
                          const espnow_frame_t *frame) {
  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  for (int i = 0; i < device_count; i++) {
    if (memcmp(devices[i].device_data.mac, info->src_addr, 6) == 0) {
      // ESP_LOGI(TAG, "Device exists: %s, updating info", mac_str);
      devices[i].rssi = info->rx_ctrl->rssi;
      devices[i].last_seen_ms = now;
      devices[i].last_type = frame->type;
      devices[i].last_seq = frame->seq;
      strncpy(devices[i].device_data.mac_str, mac_str,
              sizeof(devices[i].device_data.mac_str) - 1);
      devices[i]
          .device_data.mac_str[sizeof(devices[i].device_data.mac_str) - 1] =
          '\0';
      return;
    }
  }

  if (device_count < MAX_DEVICES) {
    ESP_LOGI(TAG, "New device detected: %s, adding to table", mac_str);
    memcpy(devices[device_count].device_data.mac, info->src_addr, 6);
    devices[device_count].rssi = info->rx_ctrl->rssi;
    devices[device_count].last_seen_ms = now;
    devices[device_count].last_type = frame->type;
    devices[device_count].last_seq = frame->seq;
    strncpy(devices[device_count].device_data.mac_str, mac_str,
            sizeof(devices[device_count].device_data.mac_str) - 1);
    devices[device_count]
        .device_data
        .mac_str[sizeof(devices[device_count].device_data.mac_str) - 1] =
        '\0'; // ensure null-termination

    device_count++;
  } else {
    ESP_LOGW(TAG, "Device table full, cannot add %s", mac_str);
  }
}

static const char *msg_type_to_str(uint8_t type) {
  switch ((msg_type_t)type) {
  case MSG_BEACON:
    return "MSG_BEACON";
  case MSG_PAIR_REQ:
    return "MSG_PAIR_REQ";
  case MSG_PAIR_ACK:
    return "MSG_PAIR_ACK";
  case MSG_PAIRED:
    return "MSG_PAIRED";
  case MSG_PAYLOAD:
    return "MSG_PAYLOAD";
  case MSG_PAYLOAD_ACK:
    return "MSG_PAYLOAD_ACK";
  default:
    return "MSG_UNKNOWN";
  }
}

esp_err_t espnow_add_peer_by_mac(const uint8_t *mac) {
  if (!mac)
    return ESP_ERR_INVALID_ARG;

  // Check if peer already exists
  if (esp_now_is_peer_exist(mac)) {
    return ESP_ERR_ESPNOW_EXIST; // already added
  }

  esp_now_peer_info_t peer = {0};

  memcpy(peer.peer_addr, mac, 6);

  peer.channel = 0;        // use current Wi-Fi channel
  peer.ifidx = WIFI_IF_AP; // interface (AP)
  peer.encrypt = false;    // no encryption

  esp_err_t res = esp_now_add_peer(&peer);
  if (res != ESP_OK) {
    ESP_LOGW("MAIN", "Failed to add peer %02X:%02X:%02X:%02X:%02X:%02X, err=%d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], res);
  } else {
    ESP_LOGI("MAIN", "Peer added: %02X:%02X:%02X:%02X:%02X:%02X", mac[0],
             mac[1], mac[2], mac[3], mac[4], mac[5]);
  }

  return res;
}

void send_to_mac(uint8_t ack_msg_type, uint8_t acked_type,
                 const uint8_t *dst_mac) {

  // Make sure peer exists
  esp_err_t res = espnow_add_peer_by_mac(dst_mac);
  if (res != ESP_OK && res != ESP_ERR_ESPNOW_EXIST) {
    ESP_LOGE(TAG, "Cannot send: peer not added");
    return;
  }

  espnow_frame_t ack = {0};
  ack.version = ESPNOW_PROTO_VERSION;
  ack.type = ack_msg_type;
  ack.ack_type = acked_type;
  //   ack.seq = seq;
  esp_wifi_get_mac(WIFI_IF_AP, ack.src);

  res = esp_now_send(dst_mac, (uint8_t *)&ack, sizeof(ack));
  if (res == ESP_OK) {
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             dst_mac[0], dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4],
             dst_mac[5]);
    ESP_LOGI(TAG, "[SENT] %s to: %s ", msg_type_to_str(ack.type), mac_str);
  } else {
    ESP_LOGW(TAG, "Failed to send: %d", res);
  }
}

void espnow_rx_cb(const esp_now_recv_info_t *info, const uint8_t *data,
                  int len) {
  if (len != sizeof(espnow_frame_t)) {
    ESP_LOGW(TAG, "[MESSAGE_REJECTED] Invalid frame size: %d", len);
    return;
  }

  const espnow_frame_t *frame = (const espnow_frame_t *)data;

  if (frame->version != ESPNOW_PROTO_VERSION) {
    ESP_LOGW(TAG, "[MESSAGE_REJECTED] Protocol version mismatch: %u",
             frame->version);
    return;
  }

  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  update_or_add_device(info, frame);

  switch (frame->type) {
  case MSG_BEACON:
    ESP_LOGI(TAG, "[RECEIVED] %s from: %s ", msg_type_to_str(frame->ack_type),
             mac_str);
    // send_to_mac(MSG_WHOIS_ACK, MSG_WHOIS_ACK, info->src_addr);
    char *json = get_devices_info_cjson(devices);

    ws_send_text(json);
    break;
  case MSG_PAIR_REQ:
    break;
  case MSG_PAIR_ACK:
    break;
  case MSG_PAIRED:
    break;
  case MSG_PAYLOAD:
    break;
  case MSG_PAYLOAD_ACK:
    break;
  default:
    ESP_LOGW(TAG, "Unhandled message type: 0x%02X", frame->type);
    break;
  }
}

void init_esp_now() {
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(TAG, "ESP-NOW Init Failed");
    return;
  }
  ESP_LOGI(TAG, "ESP-NOW initialized");

  // Set primary master key (optional, for encrypted messages)
  uint8_t pmk[16] = {0}; // all zeros is OK for unencrypted
  esp_now_set_pmk(pmk);

  // Register callback for receiving messages
  esp_now_register_recv_cb(espnow_rx_cb);
  espnow_add_peer_by_mac(broadcast_mac);
}

void send_pair_request(const uint8_t *dst_mac, uint16_t seq) {
  espnow_frame_t frame = {0};
  frame.version = ESPNOW_PROTO_VERSION;
  frame.type = MSG_PAIR_REQ;
  frame.seq = seq; // use a new sequence ID
  esp_wifi_get_mac(WIFI_IF_AP, frame.src);

  // Make sure the device is added as a peer
  if (!esp_now_is_peer_exist(dst_mac)) {
    espnow_add_peer_by_mac(dst_mac);
  }

  esp_err_t res = esp_now_send(dst_mac, (uint8_t *)&frame, sizeof(frame));
  if (res == ESP_OK) {
    ESP_LOGI(TAG,
             "[SENT][PAIR_REQ] - seq(ID)=%u to %02X:%02X:%02X:%02X:%02X:%02X",
             seq, dst_mac[0], dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4],
             dst_mac[5]);
  } else {
    ESP_LOGW(TAG, "Failed to send PAIR_REQ: %d", res);
  }
}

void remove_stale_devices_task(void *arg) {
  const uint32_t timeout_ms = 10000;

  while (1) {
    bool updated = false;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    for (int i = 0; i < device_count;) {
      if (now - devices[i].last_seen_ms > timeout_ms) {
        ESP_LOGI(TAG, "Device %s being removed",
                 devices[i].device_data.mac_str);

        for (int j = i; j < device_count - 1; j++) {
          devices[j] = devices[j + 1];
        }

        device_count--;
        updated = true;
      } else {
        i++;
      }
    }

    // ✅ Send AFTER the list is fully updated
    if (updated) {
      char *json = get_devices_info_cjson(devices);
      ws_send_text(json);
      free(json);
    }

    // ✅ ALWAYS yield to the scheduler
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
