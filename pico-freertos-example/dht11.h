#ifndef DHT11_H
#define DHT11_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/types.h"

typedef struct {
    uint8_t humidity;
    uint8_t humidity_decimal;
    uint8_t temperature;
    uint8_t temperature_decimal;
} dht11_reading_t;

typedef enum {
    DHT11_STATUS_OK = 0,
    DHT11_STATUS_NULL_READING = 1,
    DHT11_STATUS_RESPONSE_LOW_TIMEOUT = 2,
    DHT11_STATUS_RESPONSE_HIGH_TIMEOUT = 3,
    DHT11_STATUS_RESPONSE_END_TIMEOUT = 4,
    DHT11_STATUS_BIT_START_TIMEOUT = 5,
    DHT11_STATUS_BIT_END_TIMEOUT = 6,
    DHT11_STATUS_CHECKSUM_ERROR = 7
} dht11_status_t;

void dht11_init(uint pin);
bool dht11_read(uint pin, dht11_reading_t *reading);
dht11_status_t dht11_read_status(uint pin, dht11_reading_t *reading);
const char *dht11_status_string(dht11_status_t status);

#endif
