# Workflows

## When To Read This File

- Read this file for flashing, serial monitoring, frontend checks, cleanup, or crash diagnostics.

## Validation Rule

- If a firmware change affects runtime behavior on hardware, prefer `pkill -f monitor`, then `pio run -t upload`, then inspect logs with `pio device monitor`.
- For review-only, host-only, or frontend-only work, use the narrowest validation that fits the task.

## Common Commands

- Kill monitor: `pkill -f monitor`
- Upload firmware: `pio run -t upload`
- Device monitor: `pio device monitor`
- Backend tests: `pio test -e native`

## Frontend Diagnostics

1. `cd interface`
2. `npm run lint`
3. `npm run check`
4. `npm run test:run`
5. `npm run size`
6. `npm run deps:check`

Notes:

- Frontend API clients live under `interface/src/lib/services/api/`.
- Shared DTO and type definitions live under `interface/src/lib/types/`.
- Settings pages commonly compose around `interface/src/lib/utils/api/useSettings.svelte.ts`.
- Firmware build and upload run `pre:scripts/build/build_interface.py`, so frontend changes can affect embedded assets and final firmware size.

## Deep Clean

1. `pio run -t clean`
2. `rm -rf .pio`
3. `cd interface && rm -rf node_modules build .svelte-kit`
4. `cd interface && npm install`

## Core Dump Diagnostics

Use this flow when a Task Watchdog triggers or the device crashes.

1. Identify the serial port:
   ```bash
   ls /dev/cu.*
   ```
2. Read the core dump from flash:
   ```bash
   python3 -m esptool --port /dev/cu.usbmodem101 --baud 921600 read_flash 0x3F0000 0x10000 coredump_latest.bin
   ```
3. Extract the ELF from the binary:
   ```bash
   dd if=coredump_latest.bin of=coredump.elf bs=1 skip=24
   ```
4. Decode with GDB:
   ```bash
   /Users/michal/.platformio/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-gdb --batch -ex "info threads" -ex "thread apply all bt" .pio/build/waveshare_esp32s3_matrix/firmware.elf coredump.elf > coredump_report.txt
   ```
