#ifndef DEVICE_NOW_INFO_H
#define DEVICE_NOW_INFO_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_DEVICES 32

typedef struct {
  uint8_t mac[6];
  int rssi;
  char last_msg[32];
  uint32_t last_seen_ms; // timestamp in milliseconds
  uint8_t last_type;     // msg_type_t
  uint16_t last_seq;     // sequence number
  bool paired;
} device_info_t;

extern device_info_t devices[MAX_DEVICES];
extern int device_count;

#endif
