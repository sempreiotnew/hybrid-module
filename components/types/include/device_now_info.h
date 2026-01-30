#ifndef DEVICE_NOW_INFO_H
#define DEVICE_NOW_INFO_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_MAC_ADDR 6
#define MAX_NAME_LEN 16
#define MAX_DEVICES 10

typedef struct {
  uint8_t mac[6];
  char mac_str[18];
  char name[32];
} device_data_t;

typedef struct {
  device_data_t device_data;
} paired_device_t;

typedef struct {
  device_data_t device_data;
  bool paired;
  int rssi;
  char parent[18];
  char children[10][18];
  char last_msg[32];
  uint32_t last_seen_ms; // timestamp in milliseconds
  uint8_t last_type;     // msg_type_t
  uint16_t last_seq;     // sequence number
} device_info_t;

typedef struct {
  device_data_t device_data;
  paired_device_t parent;
  paired_device_t children[MAX_DEVICES];
} main_device_info_t;

extern device_info_t devices[MAX_DEVICES];
extern main_device_info_t main_device_info;
extern int device_count;

#endif
