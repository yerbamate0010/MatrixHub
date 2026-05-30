# Behavior and Availability

Navigation: [Home](../README.md) · [Special / Support Screens](README.md)

This appendix collects observable runtime rules that make MatrixHub easier to
predict during setup, recovery, and support work.

Use it when you want to understand what the device is likely to do after a
settings change, loss of connectivity, or access-level restriction.

## Wi-Fi Operating States

MatrixHub can expose network access in three practical states:

- `Manual AP-only`: the local Access Point stays up and normal Wi-Fi Station
  recovery is intentionally stopped
- normal `STA`: the device joins one of the saved Wi-Fi networks
- `Rescue AP+STA`: a local Access Point is exposed while Wi-Fi Station recovery
  continues in the background

`Manual AP-only` is the clearest recovery state because the device is no longer
trying to leave the local AP path. `Rescue AP+STA` is different: local access
returns, but MatrixHub is still trying to restore the upstream Wi-Fi
connection.

## When Each State Happens

- if `Station (STA) Mode` is set to `Offline`, MatrixHub switches to
  `Manual AP-only`
- if no saved Wi-Fi networks exist, MatrixHub also stays in `Manual AP-only`
- if saved networks exist, MatrixHub tries them automatically one by one
- if one full cycle of saved networks fails, the device waits and retries again
  later instead of giving up permanently
- if the outage lasts longer, MatrixHub can expose `Rescue AP+STA` so local
  recovery remains possible while Station mode is still retrying
- once the Wi-Fi Station connection becomes stable again, the rescue AP is
  removed and normal Station behavior resumes

## Addressing Rules

- the fallback hostname stored in Wi-Fi settings is typically `matrixhub`
- when local name resolution works, the browser address becomes
  `matrixhub.local`
- on standard first-access setups, the local AP can also use the visible SSID
  `matrixhub.local`
- a common AP fallback address is `192.168.4.1`
- the matrix IP screen can show a Station IP, an AP fallback IP, or `No WiFi`

The same `matrixhub.local` string can therefore appear in two different roles:
as a local AP name and as a browser address derived from the hostname
`matrixhub`.

## Availability Rules

- some pages stay visible in navigation even when the current user cannot open
  or change them
- notification configuration pages require management access
- `Air Mouse`, `Compensation / SCD41 Tuning`, `Keyboard`, `Mouse Jiggler`,
  `Macros`, and `USB Terminal` require management access on current builds
- `WiFi CSI` stays visible in the `WiFi` menu, but unlocks only when the
  Wi-Fi Station connection is active
- `Time` can be unavailable on builds where the NTP feature is disabled

Use these rules as a support reference when the live UI looks different from a
fully unlocked administrator session.

Navigation: [Home](../README.md) · [Special / Support Screens](README.md)
