#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "dht11.h"

typedef struct {
    bool dht_valid;
    dht11_status_t dht_status;
    dht11_reading_t dht;
    uint8_t switch_mask;
    uint8_t active_switch;
    bool switch_led_blinking;
    bool wifi_connected;
    char wifi_ip[16];
    uint32_t uptime_ms;
} app_state_snapshot_t;

void app_state_init(void);
void app_state_set_dht_reading(const dht11_reading_t *reading);
void app_state_set_dht_error(dht11_status_t status);
void app_state_set_switches(
    uint8_t switch_mask,
    uint8_t active_switch,
    bool led_blinking);
void app_state_set_wifi(bool connected, const char *ip_address);
void app_state_get_snapshot(app_state_snapshot_t *snapshot);

#endif
