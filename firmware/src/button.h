#pragma once
#include <stdbool.h>
#include <stdint.h>

struct button_state {
    bool candidate;
    bool pressed;
    uint32_t changed_at;
};

bool button_update(struct button_state *state, bool raw, uint32_t now);
