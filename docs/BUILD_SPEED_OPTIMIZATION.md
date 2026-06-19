# PlatformIO Build Speed Optimization

This is the portable checklist used for MatrixHub on Raspberry Pi 5. It is meant
to be reusable in other PlatformIO ESP32 projects without changing firmware
logic.

## Fast Path

Add a shared build cache:

```ini
[platformio]
build_cache_dir = .pio/cache
```

Keep the main embedded target explicit and avoid building every environment by
accident:

```ini
[platformio]
default_envs = your_main_env
```

Prefer `chain` LDF first:

```ini
[env:your_main_env]
lib_ldf_mode = chain
```

If `chain` fails, fix missing includes or add explicit `lib_deps` before using
`deep` or `deep+`. `deep` scans more dependency sources and can become expensive.

Exclude host-only or test-only source trees from firmware builds:

```ini
[env:your_main_env]
build_src_filter =
    +<*>
    -<native/**>
    -<test_only/**>
```

Create a developer environment for quick backend iteration. Keep release builds
separate.

```ini
[env:your_main_env_dev]
extends = env:your_main_env
custom_skip_ui = yes
```

For generated files, write only when content changed. A generator should build a
string or temporary file, compare it with the old output, and overwrite the target
only when bytes differ. This prevents headers like `version.h`, `build_info.h`,
or embedded UI headers from invalidating many C++ objects on every build.

## Helper Scripts

Useful project scripts:

```bash
scripts/build-fast.sh
scripts/build-clean.sh
scripts/build-explain.sh
```

`build-fast.sh` should default to the developer env, skip expensive generated UI
work, and use the best measured job count for the host:

```bash
PIO_ENV=your_main_env_dev PIO_JOBS=4 scripts/build-fast.sh
```

`build-explain.sh` should run SCons explain mode:

```bash
SCONSFLAGS="--debug=explain" pio run -e your_main_env
```

If the explain log only shows a PlatformIO size-check target such as
`checkprogsize`, incremental rebuilds are healthy. If it lists many `.o` files,
look for generated headers, touched source files, changed flags, or scripts that
rewrite files unconditionally.

## VS Code

If the PlatformIO extension is doing too much work on save or project switch,
use local workspace settings:

```json
{
  "platformio-ide.autoRebuildAutocompleteIndex": false,
  "platformio-ide.autoPreloadEnvTasks": false,
  "platformio-ide.activateProjectOnTextEditorChange": false
}
```

Verify these setting names against the installed extension. Some examples on the
internet reference settings that are not present in every PlatformIO/PIOArduino
extension build.

## Measurement Commands

Run the shortest useful measurement set first:

```bash
pio system info
python3 --version
pio --version
nproc
uname -a
time pio run -e your_main_env
time pio run -e your_main_env
SCONSFLAGS="--debug=explain" pio run -e your_main_env | tee build-explain.log
pio run -e your_main_env -v | tee build-verbose.log
```

Only run full clean builds when needed. On slow hosts, one baseline clean build
plus one cached clean build is usually enough.

## What Not To Do Blindly

- Do not blame PyTorch unless it appears in the actual PlatformIO process tree,
  dependency graph, scripts, or command output.
- Do not switch to `lib_ldf_mode = deep+` as a first fix.
- Do not remove safety defines, TLS defines, partition settings, or board flags
  just to make compilation faster.
- Do not add timestamps or Git hashes to global headers on every build unless the
  generated file is content-stable.
- Do not treat NVMe or microSD as the first suspect when object compilation and
  linking clearly dominate the time.

## Sources

- PlatformIO `build_cache_dir`: https://docs.platformio.org/en/latest/projectconf/sections/platformio/options/directory/build_cache_dir.html
- PlatformIO LDF modes: https://docs.platformio.org/en/latest/librarymanager/ldf.html
- PlatformIO `pio run -j`: https://docs.platformio.org/en/latest/core/userguide/cmd_run.html
- PlatformIO `build_src_filter`: https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_src_filter.html
- SCons `--debug=explain`: https://scons.org/doc/production/HTML/scons-man.html
