#include <stdint.h>

#define ESPNOW_PROTO_VERSION 0x01

typedef enum {
  MSG_BEACON = 0x01,

  MSG_PAIR_REQ = 0x02,
  MSG_PAIR_ACK = 0x03,

  MSG_PAIRED = 0x04,

  MSG_PAYLOAD = 0x05,
  MSG_PAYLOAD_ACK = 0x06,
} msg_type_t;

typedef struct __attribute__((packed)) {
  uint8_t version;
  uint8_t type;     // msg_type_t
  uint8_t ack_type; // msg_type_t being ACKed (0 if none)
  uint16_t seq;     // sequence number
  uint8_t src[6];   // sender MAC
} espnow_frame_t;