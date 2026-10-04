# Starter firmware

Blinks the green LED on PC13, reports the USER button on PA0, and gives the board a USB serial port. The SD card pins are left alone.

Compiled for STM32F411CEU6 with 512 KB flash and 128 KB RAM. It uses the board's 25 MHz crystal to run the CPU at 96 MHz and USB at 48 MHz. The button is active high and debounced for 25 ms. The LED is active high and changes state every 500 ms.

This builds, but I haven't tested it on a physical board yet.

## Build

Install a complete [Arm GNU embedded toolchain](https://developer.arm.com/tools-and-software/gnu-toolchain), including its C library. A compiler-only package without `stdint.h`, `nano.specs`, and the embedded libraries is not enough.

From this folder:

```sh
sh setup.sh
make
make test
```

If the toolchain is not on PATH:

```sh
make PREFIX=/path/to/toolchain/bin/arm-none-eabi-
```

Outputs are `build/devboard.elf`, `build/devboard.bin`, and `build/devboard.hex`. The BIN and HEX from the checked build are also in `releases/`.

`setup.sh` downloads libopencm3 from its upstream repository and checks out the revision in `libopencm3.version`. The library is LGPL-3.0-or-later; its license is included in `LIBRARY-LICENSE.txt`. The local download and build folders are ignored by Git.

## Flash over SWD

Power the board through USB-C. Connect the probe's ground to J7.4, SWDIO to J7.2, and SWCLK to J7.3. If the probe has a target voltage sense input, connect it to J7.1. NRST is available on J7.5. Check your probe's pin labels before connecting it.

With an ST-Link and OpenOCD installed:

```sh
make flash
```

The command programs the ELF, verifies it, and resets the board. It does not require a separately installed bootloader. For this application, release BOOT0 before resetting.

## USB serial

Use a USB data cable. After flashing and resetting, open the new USB serial device at 115200 baud. On macOS it should appear as `/dev/cu.usbmodem*`; Linux normally uses `/dev/ttyACM*`. The baud value is accepted for terminal compatibility; USB carries the data directly.

The terminal should show:

```text
devboard ready
Type help for commands.
```

Send a command followed by Enter:

| Command | Result |
| --- | --- |
| `help` | Lists the commands |
| `status` | Uptime, button state, press count, LED state, blink mode, and dropped-message count |
| `led on` | Stops blinking and turns the LED on |
| `led off` | Stops blinking and turns the LED off |
| `blink` | Resumes blinking |

Pressing USER prints `button pressed`; releasing it prints `button released`. RESET restarts the application and may reconnect the serial port. The terminal needs to assert DTR, which normal serial terminals do when opening the port.

USB output is buffered without waiting for a host. The LED and button keep running when the terminal is closed. If a connected host stops reading long enough to fill the buffer, the message is dropped rather than blocking the application; `status` includes the drop count.

The USB descriptor uses the development CDC identifiers `0483:5740`. Use assigned identifiers for a distributed product. The serial string is currently `DEVBOARD-001`, for the first test board.

## First board checks

1. Before connecting USB, check for a short between 3.3 V and GND.
2. Power it and measure the 3.3 V rail.
3. Flash over SWD and verify that PC13's LED blinks.
4. Open USB serial, send `status`, and try the LED commands.
5. Press and release USER. Confirm one event per action and a rising press count.
6. Reset, then disconnect and reconnect USB. Check that serial comes back and the LED still runs.

The host test checks button bounce, a held press, release, and the millisecond counter wrapping. It does not test USB enumeration or physical hardware.
