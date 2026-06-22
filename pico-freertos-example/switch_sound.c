#include "switch_sound.h"

#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "task.h"

typedef struct {
    uint16_t frequency_hz;
    uint16_t duration_ms;
    uint16_t pause_ms;
} tone_step_t;

static const tone_step_t switch_on_tune[] = {
    {784, 90, 20},
    {988, 90, 20},
    {1175, 140, 35},
    {988, 70, 20},
    {1319, 180, 0},
};

static uint sound_pin;
static TaskHandle_t sound_task_handle;

static void tone_start(uint pin, uint32_t frequency_hz) {
    if (frequency_hz == 0) {
        return;
    }

    uint slice = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    uint32_t source_hz = clock_get_hz(clk_sys);

    uint32_t divider = source_hz / (frequency_hz * 65536u) + 1u;
    if (divider < 1u) {
        divider = 1u;
    } else if (divider > 255u) {
        divider = 255u;
    }

    uint32_t wrap = source_hz / (divider * frequency_hz);
    if (wrap == 0) {
        wrap = 1;
    } else {
        --wrap;
    }
    if (wrap > 65535u) {
        wrap = 65535u;
    }

    pwm_set_clkdiv_int_frac(slice, (uint8_t)divider, 0);
    pwm_set_wrap(slice, (uint16_t)wrap);
    pwm_set_chan_level(slice, channel, (uint16_t)(wrap / 2u));
    pwm_set_enabled(slice, true);
}

static void tone_stop(uint pin) {
    uint slice = pwm_gpio_to_slice_num(pin);
    uint channel = pwm_gpio_to_channel(pin);
    pwm_set_chan_level(slice, channel, 0);
}

static void play_switch_on_tune(void) {
    for (uint index = 0;
         index < sizeof(switch_on_tune) / sizeof(switch_on_tune[0]);
         ++index) {
        const tone_step_t *step = &switch_on_tune[index];
        tone_start(sound_pin, step->frequency_hz);
        vTaskDelay(pdMS_TO_TICKS(step->duration_ms));
        tone_stop(sound_pin);

        if (step->pause_ms != 0) {
            vTaskDelay(pdMS_TO_TICKS(step->pause_ms));
        }
    }

    tone_stop(sound_pin);
}

static void sound_task(void *parameter) {
    (void)parameter;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        do {
            play_switch_on_tune();
        } while (ulTaskNotifyTake(pdTRUE, 0) != 0);
    }
}

void switch_sound_init(uint pin) {
    sound_pin = pin;

    gpio_init(sound_pin);
    gpio_set_function(sound_pin, GPIO_FUNC_PWM);
    tone_stop(sound_pin);

    BaseType_t created = xTaskCreate(
        sound_task,
        "switch-sound",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        &sound_task_handle);
    configASSERT(created == pdPASS);

    printf("Switch sound output: GPIO%u PWM\n", sound_pin);
}

void switch_sound_trigger(void) {
    if (sound_task_handle != NULL) {
        xTaskNotifyGive(sound_task_handle);
    }
}
