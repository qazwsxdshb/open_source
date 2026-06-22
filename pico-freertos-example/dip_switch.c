#include "dip_switch.h"

#include "pico/stdlib.h"

static uint8_t read_active_mask(const dip_switch_bank_t *bank) {
    uint8_t mask = 0;

    for (uint index = 0; index < DIP_SWITCH_COUNT; ++index) {
        if (!gpio_get(bank->pins[index])) {
            mask |= (uint8_t)(1u << index);
        }
    }

    return mask;
}

void dip_switch_init(
    dip_switch_bank_t *bank,
    const uint pins[DIP_SWITCH_COUNT]) {
    for (uint index = 0; index < DIP_SWITCH_COUNT; ++index) {
        bank->pins[index] = pins[index];
        gpio_init(pins[index]);
        gpio_set_dir(pins[index], GPIO_IN);
        gpio_pull_up(pins[index]);
        gpio_set_input_hysteresis_enabled(pins[index], true);
    }

    bank->stable_mask = read_active_mask(bank);
    bank->candidate_mask = bank->stable_mask;
    bank->candidate_samples = DIP_SWITCH_DEBOUNCE_SAMPLES;
}

bool dip_switch_poll(dip_switch_bank_t *bank, uint8_t *active_mask) {
    uint8_t raw_mask = read_active_mask(bank);

    if (raw_mask != bank->candidate_mask) {
        bank->candidate_mask = raw_mask;
        bank->candidate_samples = 1;
    } else if (bank->candidate_samples < DIP_SWITCH_DEBOUNCE_SAMPLES) {
        ++bank->candidate_samples;
    }

    bool changed = false;
    if (bank->candidate_samples >= DIP_SWITCH_DEBOUNCE_SAMPLES &&
        bank->stable_mask != bank->candidate_mask) {
        bank->stable_mask = bank->candidate_mask;
        changed = true;
    }

    if (active_mask != NULL) {
        *active_mask = bank->stable_mask;
    }

    return changed;
}
