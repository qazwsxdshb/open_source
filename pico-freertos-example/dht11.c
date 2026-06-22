#include "dht11.h"

#include "FreeRTOS.h"
#include "pico/stdlib.h"
#include "task.h"

enum {
    DHT11_START_SIGNAL_MS = 20,
    DHT11_RESPONSE_TIMEOUT_US = 1000,
    DHT11_BIT_TIMEOUT_US = 200,
    DHT11_ONE_THRESHOLD_US = 45
};

static bool wait_for_level(uint pin, bool level, uint32_t timeout_us) {
    uint32_t start = time_us_32();

    while (gpio_get(pin) != level) {
        if ((uint32_t)(time_us_32() - start) >= timeout_us) {
            return false;
        }
    }

    return true;
}

void dht11_init(uint pin) {
    gpio_init(pin);
    gpio_pull_up(pin);
    gpio_set_dir(pin, GPIO_IN);
}

bool dht11_read(uint pin, dht11_reading_t *reading) {
    return dht11_read_status(pin, reading) == DHT11_STATUS_OK;
}

dht11_status_t dht11_read_status(uint pin, dht11_reading_t *reading) {
    if (reading == NULL) {
        return DHT11_STATUS_NULL_READING;
    }

    uint8_t data[5] = {0};
    dht11_status_t status = DHT11_STATUS_OK;

    /*
     * DHT11 requires a host start pulse of at least 18 ms. This part does not
     * need microsecond precision, so leave interrupts enabled while waiting.
     */
    gpio_put(pin, 0);
    gpio_set_dir(pin, GPIO_OUT);
    vTaskDelay(pdMS_TO_TICKS(DHT11_START_SIGNAL_MS));

    /*
     * The response and 40 data bits take about 4 ms. Prevent the RTOS tick or
     * another task from stretching a measured high pulse.
     */
    taskENTER_CRITICAL();
    gpio_set_dir(pin, GPIO_IN);

    /* Sensor response: low for 80 us, high for 80 us, then first data low. */
    if (!wait_for_level(pin, false, DHT11_RESPONSE_TIMEOUT_US)) {
        status = DHT11_STATUS_RESPONSE_LOW_TIMEOUT;
    }
    if (status == DHT11_STATUS_OK &&
        !wait_for_level(pin, true, DHT11_RESPONSE_TIMEOUT_US)) {
        status = DHT11_STATUS_RESPONSE_HIGH_TIMEOUT;
    }
    if (status == DHT11_STATUS_OK &&
        !wait_for_level(pin, false, DHT11_RESPONSE_TIMEOUT_US)) {
        status = DHT11_STATUS_RESPONSE_END_TIMEOUT;
    }

    for (uint bit = 0; status == DHT11_STATUS_OK && bit < 40; ++bit) {
        /* Every bit starts with a roughly 50 us low pulse. */
        if (!wait_for_level(pin, true, DHT11_BIT_TIMEOUT_US)) {
            status = DHT11_STATUS_BIT_START_TIMEOUT;
            break;
        }

        uint32_t high_started = time_us_32();
        if (!wait_for_level(pin, false, DHT11_BIT_TIMEOUT_US)) {
            status = DHT11_STATUS_BIT_END_TIMEOUT;
            break;
        }

        uint32_t high_us = (uint32_t)(time_us_32() - high_started);
        data[bit / 8] <<= 1;
        if (high_us > DHT11_ONE_THRESHOLD_US) {
            data[bit / 8] |= 1;
        }
    }

    taskEXIT_CRITICAL();

    if (status != DHT11_STATUS_OK) {
        return status;
    }

    uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (checksum != data[4]) {
        return DHT11_STATUS_CHECKSUM_ERROR;
    }

    reading->humidity = data[0];
    reading->humidity_decimal = data[1];
    reading->temperature = data[2];
    reading->temperature_decimal = data[3];
    return DHT11_STATUS_OK;
}

const char *dht11_status_string(dht11_status_t status) {
    switch (status) {
        case DHT11_STATUS_OK:
            return "ok";
        case DHT11_STATUS_NULL_READING:
            return "null reading";
        case DHT11_STATUS_RESPONSE_LOW_TIMEOUT:
            return "response low timeout";
        case DHT11_STATUS_RESPONSE_HIGH_TIMEOUT:
            return "response high timeout";
        case DHT11_STATUS_RESPONSE_END_TIMEOUT:
            return "response end timeout";
        case DHT11_STATUS_BIT_START_TIMEOUT:
            return "bit start timeout";
        case DHT11_STATUS_BIT_END_TIMEOUT:
            return "bit end timeout";
        case DHT11_STATUS_CHECKSUM_ERROR:
            return "checksum error";
        default:
            return "unknown";
    }
}
