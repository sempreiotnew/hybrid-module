#include "device_now_info.h"
#include <mac_handler.h>
#include <now_protocol_websocket.h>

static const char *TAG = "now_protocol_json.c";

char *send_to_websocket(device_info_t devices[], const char *action) {
  // Top-level object
  cJSON *root_obj = cJSON_CreateObject();
  if (!root_obj)
    return NULL;

  // Add action
  cJSON_AddStringToObject(root_obj, "action", action);
  cJSON_AddStringToObject(root_obj, "mac", get_mac_str(get_chip_id()));

  // Create payload array
  cJSON *payload_array = cJSON_CreateArray();
  if (!payload_array) {
    cJSON_Delete(root_obj);
    return NULL;
  }

  for (int i = 0; i < device_count; i++) {
    cJSON *dev_obj = cJSON_CreateObject();
    if (!dev_obj)
      continue;

    cJSON_AddStringToObject(dev_obj, "mac", devices[i].device_data.mac_str);
    cJSON_AddStringToObject(dev_obj, "name", devices[i].device_data.name);
    cJSON_AddNumberToObject(dev_obj, "rssi", devices[i].rssi);
    cJSON_AddStringToObject(dev_obj, "parent", devices[i].parent);
    cJSON_AddBoolToObject(dev_obj, "paired", devices[i].paired);

    cJSON_AddItemToArray(payload_array, dev_obj);
  }

  // Attach array to root
  cJSON_AddItemToObject(root_obj, "payload", payload_array);

  // Convert to string
  char *json_str = cJSON_PrintUnformatted(root_obj);

  // Free JSON object (json_str is independent memory)
  cJSON_Delete(root_obj);

  if (json_str) {
    ws_send_text(json_str);
    free(json_str); // ✅ FIXED: NO MEMORY LEAK
  }

  return json_str; // caller must free(json_str)
}
