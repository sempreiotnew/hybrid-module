#include <stdbool.h>
#include <stdint.h>

#define MAX_MAC_ADDR 6
#define MAX_NAME_LEN 16
#define MAX_DEVICES 16 // max parents or children we can store

typedef struct {
  uint8_t mac[MAX_MAC_ADDR];
  char name[MAX_NAME_LEN]; // optional friendly name
} paired_device_t;

typedef struct {
  char name[MAX_NAME_LEN];
  // uint8_t version; // for future upgrades
  // uint8_t parent_count;
  // uint8_t child_count;
  // paired_device_t parents[MAX_DEVICES];
  // paired_device_t children[MAX_DEVICES];
} nvs_info_t;