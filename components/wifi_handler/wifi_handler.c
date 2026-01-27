#include "chip_id.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static const char *TAG = "wifi_handler.c";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);

void init_wifi_ap_sta() {

  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));

  char ap_ssid[32];

  snprintf(ap_ssid, sizeof(ap_ssid), "ESP32_%06lX",
           (unsigned long)get_chip_id());

  wifi_config_t wifi_config = {
      .ap =
          {
              .ssid_len = 0,
              .channel = 1,
              .max_connection = 4,
              .authmode = WIFI_AUTH_WPA_WPA2_PSK,
          },
  };

  strncpy((char *)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid));

  strncpy((char *)wifi_config.ap.password, "12345678",
          sizeof(wifi_config.ap.password));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI("wifi", "AP+STA started");
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {

  if (event_base == WIFI_EVENT) {
    ESP_LOGI(TAG, "WIFI EVENT ID: %d", event_id);

    switch (event_id) {

    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED");
      break;

    case WIFI_EVENT_AP_START:
      ESP_LOGW(TAG, "WIFI_EVENT_AP_START");
    default:
      break;
    }
  }

  if (event_base == IP_EVENT) {

    switch (event_id) {

    case IP_EVENT_STA_GOT_IP: {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

      ESP_LOGI(TAG, "Got IP!");
      ESP_LOGI(TAG, "IP      : " IPSTR, IP2STR(&event->ip_info.ip));
      ESP_LOGI(TAG, "Gateway : " IPSTR, IP2STR(&event->ip_info.gw));
      ESP_LOGI(TAG, "Netmask : " IPSTR, IP2STR(&event->ip_info.netmask));
      break;
    }

    case IP_EVENT_STA_LOST_IP:
      ESP_LOGW(TAG, "Lost IP");
      break;

    default:
      break;
    }
  }
}