#pragma once
#include <libopencm3/stm32/gpio.h>

#define LED_PORT GPIOC
#define LED_PIN GPIO13
#define BUTTON_PORT GPIOA
#define BUTTON_PIN GPIO0
#define USB_PORT GPIOA
#define USB_PINS (GPIO9 | GPIO11 | GPIO12)
#define BUTTON_DEBOUNCE_MS 25u
