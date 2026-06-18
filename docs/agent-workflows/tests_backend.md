# Backend Test Workflow

Run host-side Unity tests through the PlatformIO native env:

```bash
pio test -e native
```

If `pio` is not in `PATH`:

```bash
/home/test/.platformio/penv/bin/pio test -e native
```

Focused suites:

```bash
pio test -e native -f "test_ble_device_type_detector"
pio test -e native -f "test_telegram_json_parsers"
```

Coverage env:

```bash
pio test -e native_coverage
```

Add or update native tests for firmware logic that can run without ESP32
hardware. For ESP32-only behavior, document the build/log/static verification
used.
