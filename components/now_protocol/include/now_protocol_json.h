#pragma once
#include "cJSON.h"
#include "esp_log.h"
#include <stdlib.h>
char *get_devices_info_cjson(device_info_t devices[]);