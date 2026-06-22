#include "app_state.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "pico/stdlib.h"
#include "semphr.h"

static app_state_snapshot_t state;
static SemaphoreHandle_t state_mutex;

static void lock_state(void) {
    if (state_mutex != NULL) {
        xSemaphoreTake(state_mutex, portMAX_DELAY);
    }
}

static void unlock_state(void) {
    if (state_mutex != NULL) {
        xSemaphoreGive(state_mutex);
    }
}

void app_state_init(void) {
    memset(&state, 0, sizeof(state));
    state.dht_status = DHT11_STATUS_NULL_READING;
    state_mutex = xSemaphoreCreateMutex();
    configASSERT(state_mutex != NULL);
}

void app_state_set_dht_reading(const dht11_reading_t *reading) {
    if (reading == NULL) {
        app_state_set_dht_error(DHT11_STATUS_NULL_READING);
        return;
    }

    lock_state();
    state.dht_valid = true;
    state.dht_status = DHT11_STATUS_OK;
    state.dht = *reading;
    unlock_state();
}

void app_state_set_dht_error(dht11_status_t status) {
    lock_state();
    state.dht_valid = false;
    state.dht_status = status;
    unlock_state();
}

void app_state_set_switches(
    uint8_t switch_mask,
    uint8_t active_switch,
    bool led_blinking) {
    lock_state();
    state.switch_mask = switch_mask;
    state.active_switch = active_switch;
    state.switch_led_blinking = led_blinking;
    unlock_state();
}

void app_state_set_wifi(bool connected, const char *ip_address) {
    lock_state();
    state.wifi_connected = connected;
    if (ip_address == NULL) {
        state.wifi_ip[0] = '\0';
    } else {
        snprintf(state.wifi_ip, sizeof(state.wifi_ip), "%s", ip_address);
    }
    unlock_state();
}

void app_state_get_snapshot(app_state_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return;
    }

    lock_state();
    state.uptime_ms = to_ms_since_boot(get_absolute_time());
    *snapshot = state;
    unlock_state();
}
