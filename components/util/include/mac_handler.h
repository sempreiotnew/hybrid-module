#pragma once
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"

char *get_mac_str(const uint8_t *mac);
int mac_str_to_bytes(const char *mac_str, uint8_t *mac);
const uint8_t *get_chip_id(void);
void init_my_mac();