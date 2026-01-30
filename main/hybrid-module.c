#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "now_protocol.h"
#include "nvs_flash.h"
#include <constants.h>
#include <mac_handler.h>
#include <now_protocol_t.h>
#include <nvs_handler.h>
#include <stdio.h>
#include <util.h>
#include <webserver.h>
#include <wifi_handler.h>

static const char *TAG = "MAIN";

void whois_task(void *arg) {
  while (1) {
    send_beacon();
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
};

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  init_wifi_ap_sta();
  init_my_mac();
  init_nvs_data();
  init_web_server();
  init_esp_now();

  xTaskCreate(whois_task, "whois_task", 4096, NULL, 5, NULL);
  xTaskCreate(remove_stale_devices_task, "cleanup_task", 4096, NULL, 5, NULL);
}
