#ifndef TM1637_H
#define TM1637_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/types.h"

enum {
    TM1637_DIGITS = 4,
    TM1637_BRIGHTNESS_MAX = 7
};

void tm1637_init(uint clk_pin, uint dio_pin);
bool tm1637_clear(uint clk_pin, uint dio_pin);
bool tm1637_show_test(uint clk_pin, uint dio_pin);
bool tm1637_show_boot(uint clk_pin, uint dio_pin);
bool tm1637_show_run(uint clk_pin, uint dio_pin);
bool tm1637_show_read(uint clk_pin, uint dio_pin);
bool tm1637_show_switch(uint clk_pin, uint dio_pin, uint8_t switch_number);
bool tm1637_show_temperature(uint clk_pin, uint dio_pin, uint8_t temperature);
bool tm1637_show_humidity(uint clk_pin, uint dio_pin, uint8_t humidity);
bool tm1637_show_error(uint clk_pin, uint dio_pin);
bool tm1637_show_error_code(uint clk_pin, uint dio_pin, uint8_t code);

#endif
