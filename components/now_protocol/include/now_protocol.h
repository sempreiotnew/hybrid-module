void init_esp_now();
void send_beacon(void);
void remove_stale_devices_task(void *arg);
void send_pair_request(const uint8_t *dst_mac, uint16_t seq, bool is_ack);
esp_err_t delete_peer_by_mac(const uint8_t mac_addr[6]);