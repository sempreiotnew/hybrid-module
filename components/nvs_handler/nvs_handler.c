
#include "esp_log.h"
#include "nvs_flash.h"
#include <now_protocol_t.h>
#include <nvs_handler.h>

#define DEVICE_NVS_NAMESPACE "devices"

const char *TAG = "nvs_handler.c";

void set_device_state_buffer(const uint8_t *mac, const uint8_t *parent,
                             bool paired) {
  for (int i = 0; i < nearby_count; i++) {
    if (memcmp(nearby_devices_info[i].data.mac, mac, 6) == 0) {

      const char *mac_str = get_mac_str(mac);
      const char *parent_str = get_mac_str(parent);

      if (paired) {
        snprintf(nearby_devices_info[i].parent,
                 sizeof(nearby_devices_info[i].parent), "%s", parent_str);
      } else {
        nearby_devices_info[i].parent[0] = '\0';
      }
      send_to_websocket_prov(nearby_devices_info, "now_nearby_devices_info");
      ESP_LOGI(TAG, "Device marked as %s (runtime only): %s",
               paired ? "PAIRED" : "UNPAIRED", get_mac_str(mac));
      return;
    }
  }

  // If I'm the parent so add to the table
  if (memcmp(get_chip_id(), parent, 6) == 0) {

    ESP_LOGW(TAG, "ADDING PREVIOUSLY PAIRED DEVICES TO THE TABLE");
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    memcpy(nearby_devices_info[nearby_count].data.mac, mac, 6);
    const char *parent_str = get_mac_str(parent);

    snprintf(nearby_devices_info[nearby_count].parent,
             sizeof(nearby_devices_info[nearby_count].parent), "%s",
             parent_str);
    nearby_devices_info[nearby_count].rssi = 0;
    nearby_devices_info[nearby_count].last_seen_ms = now;

    strncpy(nearby_devices_info[nearby_count].data.mac_str, get_mac_str(mac),
            sizeof(devices[nearby_count].device_data.mac_str) - 1);
    nearby_devices_info[nearby_count]
        .data
        .mac_str[sizeof(nearby_devices_info[nearby_count].data.mac_str) - 1] =
        '\0'; // ensure null-termination

    nearby_count++;
  } else {
    ESP_LOGW(TAG, "Cannot mark device paired: not found in table");
  }
}

bool set_nearby_devices_info(const esp_now_recv_info_t *info,
                             const espnow_frame_t *frame) {

  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  char *mac_str = get_mac_str(info->src_addr);

  for (int i = 0; i < nearby_count; i++) {
    if (memcmp(nearby_devices_info[i].data.mac, info->src_addr, 6) == 0) {
      // ESP_LOGI(TAG, "Device exists: %s, updating info", mac_str);
      nearby_devices_info[i].rssi = info->rx_ctrl->rssi;
      nearby_devices_info[i].last_seen_ms = now;
      //   nearby_devices_info[i].last_type = frame->type;
      //   nearby_devices_info[i].last_seq = frame->seq;
      //   nearby_devices_info[i].last_seq = frame->seq;
      strncpy(nearby_devices_info[i].data.mac_str, mac_str,
              sizeof(nearby_devices_info[i].data.mac_str) - 1);
      nearby_devices_info[i]
          .data.mac_str[sizeof(nearby_devices_info[i].data.mac_str) - 1] = '\0';
      return true;
    }
  }

  return false;
}

void add_nearby_device(const esp_now_recv_info_t *info,
                       const espnow_frame_t *frame) {
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  char *mac_str = get_mac_str(info->src_addr);
  if (nearby_count < MAX_DEVICES) {
    ESP_LOGI(TAG, "New device detected NEARBY: %s, adding to table", mac_str);
    memcpy(nearby_devices_info[nearby_count].data.mac, info->src_addr, 6);
    nearby_devices_info[nearby_count].rssi = info->rx_ctrl->rssi;
    nearby_devices_info[nearby_count].last_seen_ms = now;

    strncpy(nearby_devices_info[nearby_count].data.mac_str, mac_str,
            sizeof(devices[nearby_count].device_data.mac_str) - 1);
    nearby_devices_info[nearby_count]
        .data
        .mac_str[sizeof(nearby_devices_info[nearby_count].data.mac_str) - 1] =
        '\0'; // ensure null-termination

    nearby_count++;
  } else {
    ESP_LOGW(TAG, "Device table full, cannot add %s", mac_str);
  }
}

esp_err_t nvs_save_device(const device_info_t *device) {
  nvs_handle_t handle;
  esp_err_t err;

  err = nvs_open(DEVICE_NVS_NAMESPACE, NVS_READWRITE, &handle);
  if (err != ESP_OK)
    return err;

  static char key[8]; // "dA1B2C" + '\0'

  const uint8_t *chip_id = get_chip_id();

  snprintf(key, sizeof(key), "d%02X%02X%02X", chip_id[3], chip_id[4],
           chip_id[5]);

  err = nvs_set_blob(handle, key, device, sizeof(device_info_t));
  if (err == ESP_OK) {
    err = nvs_commit(handle);
  }

  nvs_close(handle);
  return err;
}
static void make_device_key(const uint8_t mac[6], char *key, size_t key_len) {
  snprintf(key, key_len, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

esp_err_t nvs_load_device(const uint8_t mac[6], device_info_t *out_device) {
  nvs_handle_t handle;
  esp_err_t err;
  //   char key[8] = get_chip_id();
  size_t required_size = sizeof(device_info_t);

  err = nvs_open(DEVICE_NVS_NAMESPACE, NVS_READONLY, &handle);
  if (err != ESP_OK)
    return err;

  static char key[8]; // "dA1B2C" + '\0'

  const uint8_t *chip_id = get_chip_id();

  snprintf(key, sizeof(key), "d%02X%02X%02X", chip_id[3], chip_id[4],
           chip_id[5]);
  nvs_get_blob(handle, key, out_device, &required_size);
  nvs_close(handle);

  if (err == ESP_OK && required_size != sizeof(device_info_t)) {
    return ESP_ERR_INVALID_SIZE;
  }

  return err;
}

void init_nvs_data(void) {
  device_info_t dev;
  esp_err_t err;

  // Use the actual device MAC as key
  const uint8_t *mac = devices[0].device_data.mac;

  err = nvs_load_device(mac, &dev);

  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "Device not found in NVS, initializing new entry");
    dev = devices[0]; // start from runtime data

  } else if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read device from NVS (%s)", esp_err_to_name(err));
    return;
  }

  // ---- CHECK NAME ----
  if (dev.device_data.name[0] == '\0') {
    const char *mac_str = get_mac_str(get_chip_id());

    ESP_LOGI(TAG, "Device name empty, setting to MAC: %s", mac_str);

    snprintf(dev.device_data.name, sizeof(dev.device_data.name), "%s", mac_str);

    // Save updated device back to NVS
    err = nvs_save_device(&dev);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to save device to NVS (%s)", esp_err_to_name(err));
    }
  } else {
    ESP_LOGI(TAG, "Device name found in NVS: %s", dev.device_data.name);
  }

  // Sync runtime copy
  device = dev;
}
