#pragma once
#include "cJSON.h"
#include "esp_log.h"
#include "websocket.h"
#include <stdlib.h>
char *send_to_websocket(device_info_t devices[], const char *action);
char *send_to_websocket_prov(nearby_devices_t nearby_devices[],
                             const char *action);