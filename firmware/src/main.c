#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "board.h"
#include "button.h"
#include "usb_serial.h"

static volatile uint32_t ticks;
static bool blink = true;
static bool led_on;
static struct button_state button;
static uint32_t presses;

void sys_tick_handler(void)
{
    ticks++;
}

static void led_set(bool on)
{
    led_on = on;
    if (on) gpio_set(LED_PORT, LED_PIN);
    else gpio_clear(LED_PORT, LED_PIN);
}

static char *append_text(char *out, const char *text)
{
    while (*text) *out++ = *text++;
    return out;
}

static char *append_number(char *out, uint32_t number)
{
    char digits[10];
    unsigned count = 0;
    do { digits[count++] = (char)('0' + number % 10u); number /= 10u; } while (number);
    while (count) *out++ = digits[--count];
    return out;
}

static void status(void)
{
    char text[160], *p = text;
    p = append_number(append_text(p, "uptime_ms="), ticks);
    p = append_number(append_text(p, " button="), button.pressed);
    p = append_number(append_text(p, " presses="), presses);
    p = append_number(append_text(p, " led="), led_on);
    p = append_text(append_text(p, " mode="), blink ? "blink" : "manual");
    p = append_number(append_text(p, " dropped="), usb_serial_dropped());
    p = append_text(p, "\r\n");
    *p = '\0';
    usb_serial_write(text);
}

static void command(const char *line)
{
    if (!strcmp(line, "led on")) { blink = false; led_set(true); }
    else if (!strcmp(line, "led off")) { blink = false; led_set(false); }
    else if (!strcmp(line, "blink")) blink = true;
    else if (!strcmp(line, "status")) { status(); return; }
    else if (!strcmp(line, "help")) {
        usb_serial_write("help | status | led on | led off | blink\r\n"); return;
    } else { usb_serial_write("Unknown command. Type help.\r\n"); return; }
    usb_serial_write("ok\r\n");
}

int main(void)
{
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_96MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_OTGFS);
    gpio_clear(LED_PORT, LED_PIN);
    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
    gpio_set_output_options(LED_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED_PIN);
    gpio_mode_setup(BUTTON_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, BUTTON_PIN);
    gpio_mode_setup(USB_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, USB_PINS);
    gpio_set_af(USB_PORT, GPIO_AF10, USB_PINS);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(96000u - 1u);
    systick_interrupt_enable();
    systick_counter_enable();
    usb_serial_init();

    uint32_t last_blink = 0;
    bool was_open = false;
    char line[64];
    unsigned used = 0;
    bool overflow = false;
    for (;;) {
        usb_serial_poll();
        uint32_t now = ticks;
        bool open = usb_serial_open();
        if (open && !was_open) {
            usb_serial_write("devboard ready\r\nType help for commands.\r\n");
            used = 0; overflow = false;
        }
        was_open = open;
        if (blink && (uint32_t)(now - last_blink) >= 500u) {
            last_blink = now; led_set(!led_on);
        }
        if (button_update(&button, gpio_get(BUTTON_PORT, BUTTON_PIN) != 0, now)) {
            if (button.pressed) presses++;
            usb_serial_write(button.pressed ? "button pressed\r\n" : "button released\r\n");
        }
        char ch;
        while (usb_serial_getc(&ch)) {
            if (ch == '\r' || ch == '\n') {
                if (overflow) usb_serial_write("Command too long.\r\n");
                else if (used) { line[used] = '\0'; command(line); }
                used = 0; overflow = false;
            } else if (ch == '\b' || ch == 127) {
                if (used) used--;
            } else if (ch >= 32 && ch <= 126 && !overflow) {
                if (used < sizeof line - 1u) line[used++] = ch;
                else overflow = true;
            }
        }
    }
}
