void init_esp_now();
void send_beacon(void);
void remove_stale_devices_task(void *arg);
void send_pair_request(const uint8_t *dst_mac, uint16_t seq, bool is_ack);
void send_pair_ack(const uint8_t *dst_mac);
void send_unpair_request(const uint8_t *dst_mac, uint16_t seq, bool is_ack);
void send_unpair_ack(const uint8_t *dst_mac);
esp_err_t delete_peer_by_mac(const uint8_t mac_addr[6]);