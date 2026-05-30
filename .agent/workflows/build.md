---
description: How to build and verify code for the ESP32 project
---

# CRITICAL RULE: DEFAULT TO UPLOAD

Default workflow for firmware changes that need device validation:

- kill monitor if needed
- run `pio run -t upload`

This avoids wasting time on a separate build step before the same upload.

## Fast Backend Iteration

For backend-only iteration, you may skip the Svelte build:

```bash
cd /Users/michal/Documents/PlatformIO/Projects/c6 && SKIP_UI=1 pio run -e waveshare_esp32s3_matrix
```

For backend-only upload:

```bash
cd /Users/michal/Documents/PlatformIO/Projects/c6 && SKIP_UI=1 pio run -e waveshare_esp32s3_matrix -t upload
```

`SKIP_UI=1` is supported by `scripts/build/build_interface.py` and skips the frontend embed/build step.

## Steps

Use one of these, depending on intent.

1. Default firmware upload:
```bash
cd /Users/michal/Documents/PlatformIO/Projects/c6 && pio run -t upload 2>&1 | tail -25
```

2. Fast backend-only build:
```bash
cd /Users/michal/Documents/PlatformIO/Projects/c6 && SKIP_UI=1 pio run -e waveshare_esp32s3_matrix
```

3. Fast backend-only upload:
```bash
cd /Users/michal/Documents/PlatformIO/Projects/c6 && SKIP_UI=1 pio run -e waveshare_esp32s3_matrix -t upload 2>&1 | tail -25
```
