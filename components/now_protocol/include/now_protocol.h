void init_esp_now();
void send_to_mac(uint8_t ack_msg_type, uint8_t acked_type,
                 const uint8_t *dst_mac);