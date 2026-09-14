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