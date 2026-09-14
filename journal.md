# Journal for my devboard

## Devlog 01: init and organizing repo and structure and KiCad project

Date: Sep 14
Time: 30 minutes

### what was done
- Created repo at `Documents/kicad/devboard` with `git init`
- Added `.gitignore` for KiCad + macOS
- Created structure: root KiCad project + `docs/devlog/`
- Created empty KiCad project `devboard` (`devboard.kicad_pro`, `devboard.kicad_sch`, `devboard.kicad_pcb`)
- Added and wrote `README.md`

### whats next
- decide MCU part + features + size target + layer stack

## Devlog 02:  decided the MCU and board spec.

Date: Sep 14
Time: 20 minutes

- MCU STM32F411CEU6 (UFQFPN48, 100MHz, USB FS, SDIO).
- USB-C native, no UART bridge, SWD for debug.
- microSD on SDMMC 4-bit.
- 4-layer, headers, smallest practical with clean USB.
- Next: schematic blocks (power, USB, clock, SWD, SD, GPIO).

## Devlog 03: completed the schematic!

Date: Sep 14
Time: 1 hour 30 minutes

Lapse link: [lapse](https://lapse.hackclub.com/timelapse/VmLhmJkZRPre)

I made a connection list so I have something to refer to while i made the schematic (that's why the lapse showed i kept alt tabbing), made the schematic, and after that i had to fix some things like missing capacitor gnds, stray gnd near header, and power flags.

Built: STM32F411CEU6 core with decoupling, AP2112K-3.3 power from USB-C,
USB-C with CC resistors and ESD, 25MHz crystal, NRST/BOOT0/USER buttons,
green LED, SWD plus debug header, microSD, GPIO broken out to headers.

Decisions: AP2112K over smaller LDOs for SD current peaks. SD runs in SPI mode
because SDIO_CMD needs PD2, which this package does not have.

Check: ERC 0 errors, netlist verified pin by pin, 45 parts, 50 nets.

Open for layout: microSD socket part number, header pitch, button and
crystal packages, then footprints and placement.