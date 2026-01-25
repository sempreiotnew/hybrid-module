#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <webserver.h>
#include <wifi_handler.h>

static const char *TAG = "MAIN";

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  init_wifi_ap_sta();
  init_web_server();
}
