#include "tm1637.h"

#include "pico/stdlib.h"

enum {
    TM1637_BIT_DELAY_US = 10,
    TM1637_COMMAND_DATA_AUTO = 0x40,
    TM1637_COMMAND_ADDRESS = 0xc0,
    TM1637_COMMAND_DISPLAY_ON = 0x88,
    TM1637_DEFAULT_BRIGHTNESS = 3
};

static const uint8_t digit_segments[10] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66,
    0x6d, 0x7d, 0x07, 0x7f, 0x6f
};

static void line_low(uint pin) {
    gpio_put(pin, 0);
    gpio_set_dir(pin, GPIO_OUT);
}

static void line_release(uint pin) {
    gpio_set_dir(pin, GPIO_IN);
}

static void tm1637_start(uint clk_pin, uint dio_pin) {
    line_release(dio_pin);
    line_release(clk_pin);
    sleep_us(TM1637_BIT_DELAY_US);
    line_low(dio_pin);
    sleep_us(TM1637_BIT_DELAY_US);
    line_low(clk_pin);
}

static void tm1637_stop(uint clk_pin, uint dio_pin) {
    line_low(clk_pin);
    line_low(dio_pin);
    sleep_us(TM1637_BIT_DELAY_US);
    line_release(clk_pin);
    sleep_us(TM1637_BIT_DELAY_US);
    line_release(dio_pin);
    sleep_us(TM1637_BIT_DELAY_US);
}

static bool tm1637_write_byte(uint clk_pin, uint dio_pin, uint8_t value) {
    for (uint bit = 0; bit < 8; ++bit) {
        line_low(clk_pin);
        if ((value & 1u) != 0) {
            line_release(dio_pin);
        } else {
            line_low(dio_pin);
        }
        sleep_us(TM1637_BIT_DELAY_US);
        line_release(clk_pin);
        sleep_us(TM1637_BIT_DELAY_US);
        value >>= 1;
    }

    line_low(clk_pin);
    line_release(dio_pin);
    sleep_us(TM1637_BIT_DELAY_US);
    line_release(clk_pin);
    sleep_us(TM1637_BIT_DELAY_US);
    bool acknowledged = !gpio_get(dio_pin);
    line_low(clk_pin);
    line_release(dio_pin);
    return acknowledged;
}

static bool tm1637_write_command(uint clk_pin, uint dio_pin, uint8_t command) {
    tm1637_start(clk_pin, dio_pin);
    bool acknowledged = tm1637_write_byte(clk_pin, dio_pin, command);
    tm1637_stop(clk_pin, dio_pin);
    return acknowledged;
}

static bool tm1637_write_segments(
    uint clk_pin,
    uint dio_pin,
    const uint8_t segments[TM1637_DIGITS]) {
    bool acknowledged = tm1637_write_command(
        clk_pin, dio_pin, TM1637_COMMAND_DATA_AUTO);

    tm1637_start(clk_pin, dio_pin);
    acknowledged &= tm1637_write_byte(
        clk_pin, dio_pin, TM1637_COMMAND_ADDRESS);
    for (uint digit = 0; digit < TM1637_DIGITS; ++digit) {
        acknowledged &= tm1637_write_byte(clk_pin, dio_pin, segments[digit]);
    }
    tm1637_stop(clk_pin, dio_pin);

    acknowledged &= tm1637_write_command(
        clk_pin,
        dio_pin,
        TM1637_COMMAND_DISPLAY_ON | TM1637_DEFAULT_BRIGHTNESS);
    return acknowledged;
}

static uint8_t tens(uint8_t value) {
    return digit_segments[(value / 10) % 10];
}

static uint8_t units(uint8_t value) {
    return digit_segments[value % 10];
}

void tm1637_init(uint clk_pin, uint dio_pin) {
    gpio_init(clk_pin);
    gpio_init(dio_pin);
    gpio_pull_up(clk_pin);
    gpio_pull_up(dio_pin);
    gpio_set_drive_strength(clk_pin, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_drive_strength(dio_pin, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_slew_rate(clk_pin, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(dio_pin, GPIO_SLEW_RATE_SLOW);
    line_release(clk_pin);
    line_release(dio_pin);
}

bool tm1637_clear(uint clk_pin, uint dio_pin) {
    const uint8_t segments[TM1637_DIGITS] = {0, 0, 0, 0};
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}

bool tm1637_show_test(uint clk_pin, uint dio_pin) {
    const uint8_t segments[TM1637_DIGITS] = {0x7f, 0x7f, 0x7f, 0x7f};
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}

bool tm1637_show_boot(uint clk_pin, uint dio_pin) {
    const uint8_t segment_b = 0x7c;
    const uint8_t segment_o = 0x5c;
    const uint8_t segment_t = 0x78;
    uint8_t segments[TM1637_DIGITS] = {
        segment_b,
        segment_o,
        segment_o,
        segment_t
    };
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}

bool tm1637_show_run(uint clk_pin, uint dio_pin) {
    const uint8_t segment_r = 0x50;
    const uint8_t segment_u = 0x1c;
    const uint8_t segment_n = 0x54;
    uint8_t segments[TM1637_DIGITS] = {
        segment_r,
        segment_u,
        segment_n,
        0
    };
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}

bool tm1637_show_read(uint clk_pin, uint dio_pin) {
    const uint8_t segment_r = 0x50;
    const uint8_t segment_e = 0x79;
    const uint8_t segment_a = 0x77;
    const uint8_t segment_d = 0x5e;
    uint8_t segments[TM1637_DIGITS] = {
        segment_r,
        segment_e,
        segment_a,
        segment_d
    };
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}

bool tm1637_show_switch(
    uint clk_pin,
    uint dio_pin,
    uint8_t switch_number) {
    const uint8_t segment_s = 0x6d;
    const uint8_t segment_o = 0x5c;
    const uint8_t segment_n = 0x54;
    uint8_t digit = switch_number <= 9
        ? digit_segments[switch_number]
        : 0;
    uint8_t segments[TM1637_DIGITS] = {
        segment_s,
        digit,
        segment_o,
        segment_n
    };
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}

bool tm1637_show_temperature(uint clk_pin, uint dio_pin, uint8_t temperature) {
    const uint8_t segment_t = 0x78;
    const uint8_t segment_c = 0x39;
    uint8_t segments[TM1637_DIGITS] = {
        segment_t,
        tens(temperature),
        units(temperature),
        segment_c
    };
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}

bool tm1637_show_humidity(uint clk_pin, uint dio_pin, uint8_t humidity) {
    const uint8_t segment_h = 0x76;
    uint8_t segments[TM1637_DIGITS] = {
        segment_h,
        0,
        tens(humidity),
        units(humidity)
    };
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}

bool tm1637_show_error(uint clk_pin, uint dio_pin) {
    const uint8_t segment_e = 0x79;
    const uint8_t segment_r = 0x50;
    uint8_t segments[TM1637_DIGITS] = {
        segment_e,
        segment_r,
        segment_r,
        0
    };
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}

bool tm1637_show_error_code(uint clk_pin, uint dio_pin, uint8_t code) {
    const uint8_t segment_e = 0x79;
    const uint8_t segment_r = 0x50;
    uint8_t segments[TM1637_DIGITS] = {
        segment_e,
        segment_r,
        segment_r,
        code <= 9 ? digit_segments[code] : 0
    };
    return tm1637_write_segments(clk_pin, dio_pin, segments);
}
