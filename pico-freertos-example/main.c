#include <stdio.h>

#include "FreeRTOS.h"
#include "app_state.h"
#include "dht11.h"
#include "dip_switch.h"
#include "pico/stdlib.h"
#include "switch_sound.h"
#include "task.h"
#include "tm1637.h"
#include "web_server.h"

#ifndef DHT11_DATA_PIN
#define DHT11_DATA_PIN 16
#endif

#ifndef TM1637_CLK_PIN
#define TM1637_CLK_PIN 14
#endif

#ifndef TM1637_DIO_PIN
#define TM1637_DIO_PIN 15
#endif

#ifndef SWITCH_LED_PIN
#define SWITCH_LED_PIN 21
#endif

#ifndef SWITCH_SOUND_PIN
#define SWITCH_SOUND_PIN 22
#endif

enum {
    SENSOR_STARTUP_DELAY_MS = 1500,
    DISPLAY_INTERVAL_MS = 1000,
    SWITCH_POLL_MS = 50,
    LED_BLINK_MS = 250
};

static const uint DIP_SWITCH_PINS[DIP_SWITCH_COUNT] = {
    17,
    18,
    19,
    20
};

static void switch_led_init(void) {
    gpio_init(SWITCH_LED_PIN);
    gpio_set_dir(SWITCH_LED_PIN, GPIO_OUT);
    gpio_put(SWITCH_LED_PIN, 0);
}

static void switch_led_update(uint8_t active_mask) {
    if (active_mask == 0) {
        gpio_put(SWITCH_LED_PIN, 0);
        return;
    }

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    bool led_on = ((now_ms / LED_BLINK_MS) & 1u) != 0;
    gpio_put(SWITCH_LED_PIN, led_on);
}

static uint8_t first_active_switch_number(uint8_t active_mask) {
    for (uint8_t index = 0; index < DIP_SWITCH_COUNT; ++index) {
        if ((active_mask & (uint8_t)(1u << index)) != 0) {
            return (uint8_t)(index + 1);
        }
    }

    return 0;
}

static uint8_t poll_switches_and_update_outputs(
    dip_switch_bank_t *switches,
    uint8_t *last_switch_mask) {
    uint8_t active_mask = 0;
    dip_switch_poll(switches, &active_mask);

    if (active_mask != *last_switch_mask) {
        uint8_t previous_mask = *last_switch_mask;
        uint8_t newly_on_mask = (uint8_t)(active_mask & ~previous_mask);

        if (active_mask == 0) {
            printf("DIP switches: all OFF, returning to DHT11 display\n");
        } else {
            printf(
                "DIP switches: mask=0x%02x, showing switch %u\n",
                active_mask,
                first_active_switch_number(active_mask));
        }

        if (previous_mask != 0xff && newly_on_mask != 0) {
            printf("DIP switches: newly ON mask=0x%02x, playing sound\n",
                   newly_on_mask);
            switch_sound_trigger();
        }

        *last_switch_mask = active_mask;
    }

    switch_led_update(active_mask);

    uint8_t switch_number = first_active_switch_number(active_mask);
    app_state_set_switches(active_mask, switch_number, active_mask != 0);

    if (switch_number != 0) {
        tm1637_show_switch(
            TM1637_CLK_PIN,
            TM1637_DIO_PIN,
            switch_number);
    }

    return active_mask;
}

static uint8_t delay_with_switch_poll(
    uint32_t delay_ms,
    dip_switch_bank_t *switches,
    uint8_t *last_switch_mask) {
    uint8_t active_mask = 0;

    for (uint32_t elapsed_ms = 0; elapsed_ms < delay_ms;) {
        active_mask = poll_switches_and_update_outputs(
            switches,
            last_switch_mask);

        uint32_t remaining_ms = delay_ms - elapsed_ms;
        uint32_t step_ms = remaining_ms < SWITCH_POLL_MS
            ? remaining_ms
            : SWITCH_POLL_MS;
        vTaskDelay(pdMS_TO_TICKS(step_ms));
        elapsed_ms += step_ms;
    }

    return active_mask;
}

static void sensor_display_task(void *parameter) {
    (void)parameter;

    tm1637_show_run(TM1637_CLK_PIN, TM1637_DIO_PIN);
    switch_led_init();

    dip_switch_bank_t switches;
    dip_switch_init(&switches, DIP_SWITCH_PINS);

    dht11_init(DHT11_DATA_PIN);

    printf(
        "DHT11/TM1637 test started: DHT11=GPIO%u, "
        "TM1637 CLK=GPIO%u DIO=GPIO%u, "
        "DIP=GPIO%u/%u/%u/%u active-low, LED=GPIO%u, SOUND=GPIO%u\n",
        DHT11_DATA_PIN,
        TM1637_CLK_PIN,
        TM1637_DIO_PIN,
        DIP_SWITCH_PINS[0],
        DIP_SWITCH_PINS[1],
        DIP_SWITCH_PINS[2],
        DIP_SWITCH_PINS[3],
        SWITCH_LED_PIN,
        SWITCH_SOUND_PIN);

    vTaskDelay(pdMS_TO_TICKS(SENSOR_STARTUP_DELAY_MS));

    uint8_t last_switch_mask = 0xff;

    for (;;) {
        dht11_reading_t reading;

        if (poll_switches_and_update_outputs(&switches, &last_switch_mask) != 0) {
            vTaskDelay(pdMS_TO_TICKS(SWITCH_POLL_MS));
            continue;
        }

        tm1637_show_read(TM1637_CLK_PIN, TM1637_DIO_PIN);
        if (delay_with_switch_poll(
                250,
                &switches,
                &last_switch_mask) != 0) {
            continue;
        }

        if (poll_switches_and_update_outputs(&switches, &last_switch_mask) != 0) {
            vTaskDelay(pdMS_TO_TICKS(SWITCH_POLL_MS));
            continue;
        }

        dht11_status_t status =
            dht11_read_status(DHT11_DATA_PIN, &reading);
        if (status != DHT11_STATUS_OK) {
            printf(
                "DHT11: %s (%u)\n",
                dht11_status_string(status),
                (unsigned)status);
            app_state_set_dht_error(status);
            tm1637_show_error_code(
                TM1637_CLK_PIN,
                TM1637_DIO_PIN,
                (uint8_t)status);
            delay_with_switch_poll(
                2000,
                &switches,
                &last_switch_mask);
            continue;
        }

        printf(
            "DHT11: temperature=%u.%u C, humidity=%u.%u %%\n",
            reading.temperature,
            reading.temperature_decimal,
            reading.humidity,
            reading.humidity_decimal);
        app_state_set_dht_reading(&reading);

        bool acknowledged = tm1637_show_temperature(
            TM1637_CLK_PIN,
            TM1637_DIO_PIN,
            reading.temperature);
        printf(
            "TM1637: temperature display, ACK=%s\n",
            acknowledged ? "yes" : "no");
        if (delay_with_switch_poll(
                DISPLAY_INTERVAL_MS,
                &switches,
                &last_switch_mask) != 0) {
            continue;
        }

        acknowledged = tm1637_show_humidity(
            TM1637_CLK_PIN,
            TM1637_DIO_PIN,
            reading.humidity);
        printf(
            "TM1637: humidity display, ACK=%s\n",
            acknowledged ? "yes" : "no");
        delay_with_switch_poll(
            DISPLAY_INTERVAL_MS,
            &switches,
            &last_switch_mask);
    }
}

void vAssertCalled(const char *file, int line) {
    taskDISABLE_INTERRUPTS();
    printf("FreeRTOS assertion failed: %s:%d\n", file, line);
    tm1637_show_error(TM1637_CLK_PIN, TM1637_DIO_PIN);

    for (;;) {
        tight_loop_contents();
    }
}

void vApplicationMallocFailedHook(void) {
    vAssertCalled(__FILE__, __LINE__);
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name) {
    (void)task;
    (void)task_name;
    vAssertCalled(__FILE__, __LINE__);
}

int main(void) {
    stdio_init_all();
    sleep_ms(1000);
    app_state_init();

    tm1637_init(TM1637_CLK_PIN, TM1637_DIO_PIN);
    switch_sound_init(SWITCH_SOUND_PIN);

    bool acknowledged = tm1637_show_test(
        TM1637_CLK_PIN,
        TM1637_DIO_PIN);
    printf(
        "TM1637 startup test: CLK=GPIO%u, DIO=GPIO%u, ACK=%s\n",
        TM1637_CLK_PIN,
        TM1637_DIO_PIN,
        acknowledged ? "yes" : "no");
    sleep_ms(750);
    tm1637_show_boot(TM1637_CLK_PIN, TM1637_DIO_PIN);
    sleep_ms(750);

    BaseType_t created = xTaskCreate(
        sensor_display_task,
        "dht11-display",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL);
    configASSERT(created == pdPASS);

    web_server_start();

    tm1637_show_run(TM1637_CLK_PIN, TM1637_DIO_PIN);
    sleep_ms(750);

    printf("Starting FreeRTOS scheduler\n");
    vTaskStartScheduler();

    vAssertCalled(__FILE__, __LINE__);
    return 0;
}
