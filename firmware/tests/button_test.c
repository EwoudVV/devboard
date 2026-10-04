#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "button.h"

int main(void)
{
    struct button_state b = {0};
    assert(!button_update(&b, true, 10));
    assert(!button_update(&b, false, 15));
    assert(!button_update(&b, true, 20));
    assert(!button_update(&b, true, 44));
    assert(button_update(&b, true, 45));
    assert(b.pressed);
    assert(!button_update(&b, true, 500));
    assert(!button_update(&b, false, 501));
    assert(button_update(&b, false, 526));
    assert(!b.pressed);
    b = (struct button_state){0};
    assert(!button_update(&b, true, UINT32_MAX - 10u));
    assert(!button_update(&b, true, 13u));
    assert(button_update(&b, true, 14u));
    puts("button debounce, held input, release, and timer rollover passed");
}
