
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include <constants.h>
#include <device_now_info.h>
#include <mac_handler.h>
#include <now_protocol_t.h>
#include <now_protocol_websocket.h>
#include <nvs_handler.h>
#include <util.h>

static const char *TAG = "now_protocol.c";

esp_err_t espnow_add_peer_by_mac(const uint8_t *mac) {
  if (!mac)
    return ESP_ERR_INVALID_ARG;

  if (esp_now_is_peer_exist(mac)) {
    return ESP_ERR_ESPNOW_EXIST; // already added
  }

  esp_now_peer_info_t peer = {0};

  memcpy(peer.peer_addr, mac, 6);

  peer.channel = 0; // use current Wi-Fi channel
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;

  esp_err_t res = esp_now_add_peer(&peer);
  if (res != ESP_OK) {
    ESP_LOGW("MAIN", "Failed to add peer %s, err=%d", get_mac_str(mac), res);

  } else {
    ESP_LOGI("MAIN", "Peer added: %s", get_mac_str(mac));
  }

  return res;
}
void update_or_add_device(const esp_now_recv_info_t *info,
                          const espnow_frame_t *frame) {
  char *mac_str = get_mac_str(info->src_addr);

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

void send_now_msg(const uint8_t *dst_mac, espnow_frame_t data) {
  esp_err_t res = esp_now_send(dst_mac, (uint8_t *)&data, sizeof(data));
  if (res == ESP_OK) {
    ESP_LOGI(TAG, "type=%s seq=%u to %s", msg_type_to_str(data.type), data.seq,
             get_mac_str(dst_mac));

  } else {
    ESP_LOGW(TAG, "Failed to send msg %d to %s", res, get_mac_str(dst_mac));
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
    ESP_LOGI(TAG, "[SENT] type=%s to FF:FF:FF:FF:FF:FF",
             msg_type_to_str(frame.type));
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

void send_pair_ack(const uint8_t *dst_mac) {
  send_pair_request(dst_mac, 10, true);
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

void send_unpair_ack(const uint8_t *dst_mac) {
  send_unpair_request(dst_mac, 10, true);
}

esp_err_t delete_peer_by_mac(const uint8_t mac_addr[6]) {
  if (!mac_addr) {
    ESP_LOGE(TAG, "MAC address is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = esp_now_del_peer(mac_addr);
  if (ret == ESP_OK) {
    char *mac_str = get_mac_str(mac_addr);
    ESP_LOGI(TAG, "Peer deleted successfully: %s", mac_str);
  } else if (ret == ESP_ERR_ESPNOW_NOT_FOUND) {
    ESP_LOGW(TAG, "Peer not found in list");
  } else {
    ESP_LOGE(TAG, "Failed to delete peer: %s", esp_err_to_name(ret));
  }

  return ret;
}

bool children_add(char children[10][18], const char *mac_str) {
  // Check if already exists
  for (int i = 0; i < 10; i++) {
    if (strcmp(children[i], mac_str) == 0) {
      return true; // already present
    }
  }

  // Find empty slot
  for (int i = 0; i < 10; i++) {
    if (children[i][0] == '\0') {
      snprintf(children[i], 18, "%s", mac_str);
      return true;
    }
  }

  // No space
  return false;
}

bool children_remove(char children[10][18], const char *mac_str) {
  for (int i = 0; i < 10; i++) {
    if (strcmp(children[i], mac_str) == 0) {
      children[i][0] = '\0'; // mark empty
      return true;
    }
  }
  return false; // not found
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
  char *mac_str = get_mac_str(info->src_addr);

  char src[18], dst_addr[18], dst[18];
  mac_to_str(info->src_addr, src);
  mac_to_str(info->des_addr, dst_addr);
  mac_to_str(frame->dst, dst);

  ESP_LOGI(TAG, "type:%s src:%s dst_addr:%s dst:%s",
           msg_type_to_str(frame->type), src, dst_addr, dst);

  if (broadcast && frame->type == MSG_BEACON) {
    if (!set_nearby_devices_info(info, frame)) { // If device is not in the list
      add_nearby_device(info, frame);
    }
    send_to_websocket_prov(nearby_devices_info, "now_nearby_devices_info");

    return;
  }

  switch (frame->type) {

  case MSG_PAIR_REQ:
    if (for_me) {
      ESP_LOGI(TAG, "[RECEIVED] %s from: %s PASS: %s ",
               msg_type_to_str(frame->type), mac_str, frame->password);
      if (strcmp(frame->password, "1234") == 0) {
        device_info_t dev;
        ESP_LOGI(TAG, "AUTHORIZED!!");
        espnow_add_peer_by_mac(info->src_addr);
        send_pair_ack(info->src_addr);
        set_nearby_devices_info_buffer(info->src_addr, frame->dst, false);

        nvs_load_device(frame->dst, &dev);
        children_add(dev.children, get_mac_str(info->src_addr));
        nvs_save_device(&dev);
        for (int i = 0; i < 10; i++) {
          if (dev.children[i][0] != '\0') {
            ESP_LOGI(TAG, "Child %d: %s", i, dev.children[i]);
          }
        }

      } else {
        ESP_LOGW(TAG, "NOT AUTHORIZED!!");
      }
    }

    break;
  case MSG_PAIR_ACK:
    if (for_me) {
      device_info_t dev;
      ESP_LOGI(TAG, "[RECEIVED] %s from: %s  ", msg_type_to_str(frame->type),
               mac_str);
      espnow_add_peer_by_mac(info->src_addr);
      set_nearby_devices_info_buffer(info->src_addr, get_chip_id(), true);

      nvs_load_device(frame->dst, &dev);

      children_add(dev.children, get_mac_str(info->src_addr));
      nvs_save_device(&dev);
      for (int i = 0; i < 10; i++) {
        if (dev.children[i][0] != '\0') {
          ESP_LOGI(TAG, "Child %d: %s", i, dev.children[i]);
        }
      }
    }

    break;
  case MSG_UNPAIR_REQ:
    ESP_LOGI(TAG, "[RECEIVED] %s from: %s PASS: %s ",
             msg_type_to_str(frame->type), mac_str, frame->password);

    device_info_t dev;
    send_unpair_ack(info->src_addr);
    set_nearby_devices_info_buffer(info->src_addr, frame->dst, false);
    nvs_load_device(frame->dst, &dev);
    children_remove(dev.children, get_mac_str(info->src_addr));
    nvs_save_device(&dev);
    for (int i = 0; i < 10; i++) {
      if (dev.children[i][0] != '\0') {
        ESP_LOGI(TAG, "Child  %d: %s", i, dev.children[i]);
      }
    }
    break;
  case MSG_UNPAIR_ACK:
    ESP_LOGI(TAG, "[RECEIVED] %s from: %s  ", msg_type_to_str(frame->type),
             mac_str);
    // delete_peer_by_mac(info->src_addr);
    set_nearby_devices_info_buffer(info->src_addr, get_chip_id(), false);
    nvs_load_device(frame->dst, &dev);
    children_remove(dev.children, get_mac_str(info->src_addr));
    nvs_save_device(&dev);
    for (int i = 0; i < 10; i++) {
      if (dev.children[i][0] != '\0') {
        ESP_LOGI(TAG, "Child  %d: %s", i, dev.children[i]);
      }
    }

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
  for (int i = 0; i < 10; i++) {
    if (device.children[i][0] != '\0') {
      uint8_t mac[6]; // <-- allocate memory
      mac_str_to_bytes(device.children[i], mac);
      espnow_add_peer_by_mac(mac);
      set_nearby_devices_info_buffer(mac, get_chip_id(), true);
      ESP_LOGI(TAG, "PEER CHILD ADDED ! %s", device.children[i]);
    }
  }
}

void remove_stale_devices_task(void *arg) {
  const uint32_t timeout_ms = 10000;

  while (1) {
    bool updated = false;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    for (int i = 0; i < nearby_count;) {
      if (now - nearby_devices_info[i].last_seen_ms > timeout_ms) {
        ESP_LOGI(TAG, "Device %s being removed",
                 nearby_devices_info[i].data.mac_str);

        for (int j = i; j < nearby_count - 1; j++) {
          nearby_devices_info[j] = nearby_devices_info[j + 1];
        }

        nearby_count--;
        updated = true;
      } else {
        i++;
      }
    }

    // ✅ Send AFTER the list is fully updated
    if (updated) {
      send_to_websocket_prov(nearby_devices_info, "now_nearby_devices_info");
    }

    // ✅ ALWAYS yield to the scheduler
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
