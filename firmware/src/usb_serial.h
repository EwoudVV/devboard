#pragma once
#include <stdbool.h>
#include <stdint.h>

void usb_serial_init(void);
void usb_serial_poll(void);
bool usb_serial_open(void);
bool usb_serial_write(const char *text);
bool usb_serial_getc(char *ch);
uint32_t usb_serial_dropped(void);
