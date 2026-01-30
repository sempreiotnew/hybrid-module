
#include <nvs_handler.h>

const char *TAG = "nvs_handler.c";

void set_device_state_buffer(const uint8_t *mac, bool paired) {
  for (int i = 0; i < device_count; i++) {
    if (memcmp(devices[i].device_data.mac, mac, 6) == 0) {

      const char *mac_str = get_mac_str(mac);
      devices[i].paired = paired;
      if (paired) {
        snprintf(devices[i].parent, sizeof(devices[i].parent), "%s", mac_str);
      } else {
        devices[i].parent[0] = '\0';
      }
      send_to_websocket(devices, "now_nearby_devices_info");
      ESP_LOGI(TAG, "Device marked as %s (runtime only): %s",
               paired ? "PAIRED" : "UNPAIRED", get_mac_str(mac));
      return;
    }
  }

  ESP_LOGW(TAG, "Cannot mark device paired: not found in table");
}