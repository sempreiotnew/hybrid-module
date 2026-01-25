
#include "esp_mac.h"
#include "esp_system.h"

uint32_t get_chip_id() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  // Arduino ESP32 uses last 3 bytes of base MAC
  return ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) |
         ((uint32_t)mac[5]);
}