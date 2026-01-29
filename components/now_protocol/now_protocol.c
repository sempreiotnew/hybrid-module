
#include "constants.h"
#include "device_now_info.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "now_protocol_t.h"
#include "now_protocol_websocket.h"
#include <mac_handler.h>

static const char *TAG = "now_protocol.c";

void mark_device_paired(const uint8_t *mac, bool paired) {
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

  ESP_LOGW(TAG, "Cannot mark device paired: not found in table");
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

  peer.channel = 0;         // use current Wi-Fi channel
  peer.ifidx = WIFI_IF_STA; // interface (AP)
  peer.encrypt = false;     // no encryption

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
  case MSG_UNPAIR_REQ:
    return "MSG_UNPAIR_REQ";
  case MSG_UNPAIR_ACK:
    return "MSG_UNPAIR_ACK";
  case MSG_PAYLOAD:
    return "MSG_PAYLOAD";
  case MSG_PAYLOAD_ACK:
    return "MSG_PAYLOAD_ACK";
  default:
    return "MSG_UNKNOWN";
  }
}

void send_now_msg(const uint8_t *dst_mac, espnow_frame_t data) {
  esp_err_t res = esp_now_send(dst_mac, (uint8_t *)&data, sizeof(data));
  if (res == ESP_OK) {
    ESP_LOGI(TAG, "[SENT] type=%s seq=%u to %02X:%02X:%02X:%02X:%02X:%02X",
             msg_type_to_str(data.type), data.seq, dst_mac[0], dst_mac[1],
             dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);

  } else {
    ESP_LOGW(TAG, "Failed to send msg %d", res);
  }
}

void send_beacon(void) {
  espnow_frame_t frame = {0};

  frame.version = ESPNOW_PROTO_VERSION;
  frame.type = MSG_BEACON;
  frame.ack_type = MSG_BEACON;
  frame.seq = 0;

  memcpy(frame.dst, broadcast_mac, 6);
  memcpy(frame.src, get_chip_id(), 6);

  esp_err_t res = esp_now_send(broadcast_mac, (uint8_t *)&frame, sizeof(frame));

  if (res == ESP_OK) {
    ESP_LOGI(TAG, "[SENT] type=MSG_BEACON to FF:FF:FF:FF:FF:FF");
  } else {
    ESP_LOGW(TAG, "Failed to send BEACON: %d", res);
  }
}

void send_pair_request(const uint8_t *dst_mac, uint16_t seq, bool is_ack) {

  espnow_frame_t frame = {0};
  frame.version = ESPNOW_PROTO_VERSION;
  frame.type = is_ack ? MSG_PAIR_ACK : MSG_PAIR_REQ;
  frame.seq = seq; // use a new sequence ID

  memcpy(frame.dst, dst_mac, 6);

  strncpy(frame.password, "1234", sizeof(frame.password) - 1);

  esp_wifi_get_mac(WIFI_IF_STA, frame.src);

  send_now_msg(is_ack ? dst_mac : broadcast_mac, frame);
}

void send_unpair_request(const uint8_t *dst_mac, uint16_t seq, bool is_ack) {

  espnow_frame_t frame = {0};
  frame.version = ESPNOW_PROTO_VERSION;
  frame.type = is_ack ? MSG_UNPAIR_ACK : MSG_UNPAIR_REQ;
  frame.seq = seq; // use a new sequence ID

  memcpy(frame.dst, dst_mac, 6);

  // strncpy(frame.password, "1234", sizeof(frame.password) - 1);

  esp_wifi_get_mac(WIFI_IF_STA, frame.src);

  send_now_msg(dst_mac, frame);
}

esp_err_t delete_peer_by_mac(const uint8_t mac_addr[6]) {
  if (!mac_addr) {
    ESP_LOGE(TAG, "MAC address is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = esp_now_del_peer(mac_addr);
  if (ret == ESP_OK) {
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4],
             mac_addr[5]);
    ESP_LOGI(TAG, "Peer deleted successfully: %s", mac_str);
  } else if (ret == ESP_ERR_ESPNOW_NOT_FOUND) {
    ESP_LOGW(TAG, "Peer not found in list");
  } else {
    ESP_LOGE(TAG, "Failed to delete peer: %s", esp_err_to_name(ret));
  }

  return ret;
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

  bool for_me = memcmp(frame->dst, get_chip_id(), 6) == 0;
  bool broadcast = memcmp(frame->dst, broadcast_mac, 6) == 0;

  if (for_me || broadcast) {
    ESP_LOGI(TAG, "SRC : %s", get_mac_str(info->src_addr));
    ESP_LOGI(TAG, "DEST: %s", get_mac_str(info->des_addr));
    ESP_LOGI(TAG, "MSG: %s", get_mac_str(frame->dst));
    update_or_add_device(info, frame);
  } else {
    ESP_LOGI(TAG, "NOT FOR ME");
  }

  // ESP_LOGI(TAG, "  ==>  %s", get_mac_str(frame->dst));
  // update_or_add_device(info, frame);

  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  switch (frame->type) {
  case MSG_BEACON:
    ESP_LOGI(TAG, "[RECEIVED] %s from: %s ", msg_type_to_str(frame->ack_type),
             mac_str);
    send_to_websocket(devices, "now_nearby_devices_info");

    break;

  case MSG_PAIR_REQ:

    if (memcmp(frame->dst, get_chip_id(), 6) != 0) {
      ESP_LOGI(TAG, "PAIR_REQ not for me, ignoring");

    } else {
      ESP_LOGI(TAG, "[RECEIVED] %s from: %s PASS: %s ",
               msg_type_to_str(frame->type), mac_str, frame->password);
      if (strcmp(frame->password, "1234") == 0) {
        ESP_LOGI(TAG, "AUTHORIZED!!");
        espnow_add_peer_by_mac(info->src_addr);
        send_pair_request(info->src_addr, 10, true);
        mark_device_paired(info->src_addr, true);
      } else {
        ESP_LOGW(TAG, "NOT AUTHORIZED!!");
      }
    }

    break;
  case MSG_PAIR_ACK:
    if (memcmp(frame->dst, get_chip_id(), 6) != 0) {
      ESP_LOGI(TAG, "MSG_PAIR_ACK not for me, ignoring");

    } else {
      ESP_LOGI(TAG, "[RECEIVED] %s from: %s  ", msg_type_to_str(frame->type),
               mac_str);
      espnow_add_peer_by_mac(info->src_addr);
      mark_device_paired(info->src_addr, true);
    }

    break;
  case MSG_UNPAIR_REQ:
    ESP_LOGI(TAG, "[RECEIVED] %s from: %s PASS: %s ",
             msg_type_to_str(frame->type), mac_str, frame->password);
    send_unpair_request(info->src_addr, 10, true);
    mark_device_paired(info->src_addr, false);
    break;
  case MSG_UNPAIR_ACK:
    ESP_LOGI(TAG, "[RECEIVED] %s from: %s  ", msg_type_to_str(frame->type),
             mac_str);
    delete_peer_by_mac(info->src_addr);
    mark_device_paired(info->src_addr, false);
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
      send_to_websocket(devices, "now_nearby_devices_info");
    }

    // ✅ ALWAYS yield to the scheduler
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
