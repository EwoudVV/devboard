#include "button.h"

bool button_update(struct button_state *state, bool raw, uint32_t now)
{
    if (raw != state->candidate) {
        state->candidate = raw;
        state->changed_at = now;
    }
    if (raw != state->pressed && (uint32_t)(now - state->changed_at) >= 25u) {
        state->pressed = raw;
        return true;
    }
    return false;
}
