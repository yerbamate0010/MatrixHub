# System and Maintenance

Navigation: [Home](../README.md) · [Basic Flows](../README.md#basic-use-cases) · [Additional Flows](../README.md#additional-use-cases) · [Reference](../README.md#reference-sections)

The `System` area groups health, logs, timekeeping, display behavior, and
maintenance pages.

Treat this page as the hub for the `System` menu. Open the linked reference
pages below when you want route-level details instead of a single summary.

## Everyday Checks

These pages are the most useful during normal monitoring and setup:

- [System Status](system/status.md) for device health, Wi-Fi recovery context,
  diagnostics modals, and admin-only live log tail
- [Data Logs](system/data-logs.md) for stored sensor archives and exports
- [Time](system/time.md) for NTP status, manual time entry, and time zone
  selection
- [Matrix LED](system/matrix-led.md) for the device display, alarm mode, icons,
  and idle effects
- [Styles](system/styles.md) for browser-local theme and interface appearance

## Admin and Service Pages

These pages are mainly for calibration, maintenance, or account management:

- [Compensation / SCD41 Tuning](system/compensation.md) for stable temperature
  offset correction and live preview
- [Power Settings](system/power.md) for auto-sleep, restart, hygiene restart,
  and factory reset
- [Users](system/users.md) for local accounts and JWT secret management
- [File Manager](system/file-manager.md) for LittleFS browsing, uploads,
  downloads, and protected-path behavior

## Access and Availability Notes

- `System Status`, `Data Logs`, `Time`, `Matrix LED`, and `Styles` are the
  most likely pages to matter during day-to-day use
- `Compensation`, `Power Settings`, and `Users` require management access on
  current builds
- `File Manager` is an admin-oriented page and can also be disabled entirely by
  firmware feature flags
- `Matrix LED` is more flexible than a simple alarm light: read access can be
  enough to inspect current settings, while changes still require management
  access
- `Time` can be unavailable on builds where the NTP feature is disabled

For the built-in support hub under the same main menu area, see
[Help page](../appendix/help.md).

Navigation: [Home](../README.md) · [Basic Flows](../README.md#basic-use-cases) · [Additional Flows](../README.md#additional-use-cases) · [Reference](../README.md#reference-sections)
