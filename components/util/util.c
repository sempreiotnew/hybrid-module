
#include <now_protocol_t.h>
#include <util.h>

const char *msg_type_to_str(uint8_t type) {
  switch ((msg_type_t)type) {
  case MSG_BEACON:
    return "MSG_BEACON";
  case MSG_PAIR_REQ:
    return "MSG_PAIR_REQ";
  case MSG_PAIR_ACK:
    return "MSG_PAIR_ACK";
  case MSG_UNPAIR_REQ:
    return "MSG_UNPAIR_REQ";
  case MSG_UNPAIR_ACK:
    return "MSG_UNPAIR_ACK";
  case MSG_PAYLOAD:
    return "MSG_PAYLOAD";
  case MSG_PAYLOAD_ACK:
    return "MSG_PAYLOAD_ACK";
  default:
    return "MSG_UNKNOWN";
  }
}