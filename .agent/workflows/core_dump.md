---
description: Procedure for retrieving and decoding ESP32 core dumps
---

Follow these steps to diagnose a system crash or Task Watchdog (TWD) trigger:

1. **Identify the Serial Port**
   List available ports to find the ESP32-S3 (usually `/dev/cu.usbmodem101`):
   ```bash
   ls /dev/cu.*
   ```

2. **Retrieve Core Dump from Flash**
   Use `esptool` to read the `coredump` partition. The standard offset for this project is `0x3F0000` with size `0x10000`.
   // turbo
   ```bash
   python3 -m esptool --port /dev/cu.usbmodem101 --baud 921600 read_flash 0x3F0000 0x10000 coredump_latest.bin
   ```

3. **Prepare the ELF file**
   Extract the ELF data from the raw binary (skip the 24-byte ESP-IDF header):
   // turbo
   ```bash
   dd if=coredump_latest.bin of=coredump.elf bs=1 skip=24
   ```

4. **Decode with GDB**
   Run the specific GDB for ESP32-S3 with the firmware ELF and extracted core ELF.
   // turbo
   ```bash
   /Users/michal/.platformio/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-gdb --batch -ex "info threads" -ex "thread apply all bt" .pio/build/waveshare_esp32s3_matrix/firmware.elf coredump.elf > coredump_report.txt
   ```

5. **Analyze the Results**
   - Check `coredump_report.txt` for all thread backtraces.
   - Look for tasks blocked in I/O, long loops, or the task that triggered the panic.
   - Pay attention to `TaskWatchdog::reset()` calls in the code of the identified task.
