#ifndef DIP_SWITCH_H
#define DIP_SWITCH_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/types.h"

enum {
    DIP_SWITCH_COUNT = 4,
    DIP_SWITCH_DEBOUNCE_SAMPLES = 3
};

typedef struct {
    uint pins[DIP_SWITCH_COUNT];
    uint8_t stable_mask;
    uint8_t candidate_mask;
    uint8_t candidate_samples;
} dip_switch_bank_t;

void dip_switch_init(
    dip_switch_bank_t *bank,
    const uint pins[DIP_SWITCH_COUNT]);

/*
 * Poll at a fixed interval. Returns true when the debounced state changes.
 * A set bit means the corresponding switch is ON (pin connected to GND).
 */
bool dip_switch_poll(dip_switch_bank_t *bank, uint8_t *active_mask);

#endif
