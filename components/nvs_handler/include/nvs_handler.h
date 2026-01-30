#pragma once
#include "esp_log.h"
#include <device_now_info.h>
#include <mac_handler.h>
#include <now_protocol_websocket.h>

void set_device_state_buffer(const uint8_t *mac, bool paired);