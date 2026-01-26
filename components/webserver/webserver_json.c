#include "webserver_json.h"
static const char *TAG = "webserver_json.c";

static bool mac_str_to_bytes(const char *mac_str, uint8_t *mac_bytes) {
  if (!mac_str || !mac_bytes)
    return false;

  int values[6];
  if (sscanf(mac_str, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) != 6) {
    return false;
  }

  for (int i = 0; i < 6; i++) {
    mac_bytes[i] = (uint8_t)values[i];
  }

  return true;
}
bool parse_pair_post_content(const char *content, uint8_t mac_out[6]) {
  cJSON *res = cJSON_Parse(content);
  if (!res)
    return false;

  const cJSON *mac_item = cJSON_GetObjectItem(res, "mac");
  if (!cJSON_IsString(mac_item)) {
    cJSON_Delete(res);
    return false;
  }

  if (!mac_str_to_bytes(mac_item->valuestring, mac_out)) {
    ESP_LOGW(TAG, "Invalid MAC format: %s", mac_item->valuestring);
    cJSON_Delete(res);
    return false;
  }

  cJSON_Delete(res);
  return true;
}
