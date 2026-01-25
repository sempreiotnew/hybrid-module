
#include "esp_mac.h"
#include "esp_system.h"

uint64_t get_chip_id() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  uint64_t id = 0;
  for (int i = 0; i < 6; i++) {
    id = (id << 8) | mac[i];
  }
  return id;
}
