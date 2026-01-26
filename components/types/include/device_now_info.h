#ifndef DEVICE_NOW_INFO_H
#define DEVICE_NOW_INFO_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_MAC_ADDR 6
#define MAX_NAME_LEN 16
#define MAX_DEVICES 10

typedef struct {
  uint8_t mac[16];
  char name[30]; // optional friendly name
} paired_device_t;

typedef struct {
  uint8_t mac[6];
  char mac_str[18];
  int rssi;
  char last_msg[32];
  char name[32];
  uint32_t last_seen_ms; // timestamp in milliseconds
  uint8_t last_type;     // msg_type_t
  uint16_t last_seq;     // sequence number
  paired_device_t parents[MAX_DEVICES];
  paired_device_t children[MAX_DEVICES];
} device_info_t;

extern device_info_t devices[MAX_DEVICES];
extern int device_count;

#endif
