#include <mac_handler.h>

static uint8_t my_mac[6];
static const char *TAG = "mac_handler";

void init_my_mac(void) {
  esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, my_mac);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get MAC: %s", esp_err_to_name(err));
  }

  ESP_LOGW(TAG, "CHIPID: %u - MAC: %s", get_chip_id(),
           get_mac_str(get_chip_id()));
}

const uint8_t *get_chip_id(void) { return my_mac; }

const char *get_mac_str(const uint8_t *mac) {
  static char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);
  return mac_str;
}
