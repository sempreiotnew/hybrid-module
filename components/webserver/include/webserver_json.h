#pragma once
#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"

bool parse_pair_post_content(const char *content, uint8_t mac_out[6]);