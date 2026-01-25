#include "device_now_info.h"
#include <now_protocol_json.h>

static const char *TAG = "now_protocol_json.c";

char *get_devices_info_cjson(device_info_t devices[]) {
  // Create a JSON array
  cJSON *root_array = cJSON_CreateArray();
  if (!root_array) {
    return NULL;
  }

  for (int i = 0; i < device_count; i++) {
    cJSON *dev_obj = cJSON_CreateObject();
    if (!dev_obj) {
      continue;
    }

    cJSON_AddStringToObject(dev_obj, "mac", devices[i].mac_str);
    cJSON_AddStringToObject(dev_obj, "name", devices[i].name);
    cJSON_AddNumberToObject(dev_obj, "rssi", devices[i].rssi);

    cJSON_AddItemToArray(root_array, dev_obj);
  }

  char *json_str = cJSON_PrintUnformatted(root_array);

  cJSON_Delete(root_array);

  return json_str;
}