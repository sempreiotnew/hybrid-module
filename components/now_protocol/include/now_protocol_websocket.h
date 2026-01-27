#pragma once
#include "cJSON.h"
#include "esp_log.h"
#include "websocket.h"
#include <stdlib.h>
char *send_to_websocket(device_info_t devices[], const char *action);