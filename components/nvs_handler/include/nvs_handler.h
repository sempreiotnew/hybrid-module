#pragma once
#include "esp_log.h"
#include "esp_now.h"
#include <device_now_info.h>
#include <mac_handler.h>
#include <now_protocol_websocket.h>

void init_nvs_data();
void set_nearby_devices_info_buffer(const uint8_t *mac, const uint8_t *parent,
                                    bool paired);
bool set_nearby_devices_info(const esp_now_recv_info_t *info,
                             const espnow_frame_t *frame);
void add_nearby_device(const esp_now_recv_info_t *info,
                       const espnow_frame_t *frame);

esp_err_t nvs_load_device(const uint8_t mac[6], device_info_t *out_device);
esp_err_t nvs_save_device(const device_info_t *device);