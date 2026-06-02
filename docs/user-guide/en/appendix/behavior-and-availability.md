# Behavior and Availability

Navigation: [Home](../README.md) · [Special / Support Screens](README.md)

This appendix collects observable runtime rules that make MatrixHub easier to
predict during setup, recovery, and support work.

Use it when you want to understand what the device is likely to do after a
settings change, loss of connectivity, or access-level restriction.

## Wi-Fi Operating Modes

MatrixHub has three explicit Wi-Fi modes:

- `off`: Wi-Fi radio is disabled. Network-dependent features such as Telegram
  are unavailable.
- `ap`: Access Point only. The local AP stays up and Station reconnects are
  stopped.
- `sta`: Station only. The device joins saved Wi-Fi networks and retries with
  backoff when they fail.

`sta` never starts AP automatically after connection failures. Use the web UI,
Matrix menu, or factory reset to move back to AP when local setup access is
needed.

## When Each Mode Happens

- after a fresh flash or factory reset with no saved Wi-Fi networks, MatrixHub
  starts in `ap`
- `off` is selected explicitly from the web UI or Matrix menu
- `ap` is selected explicitly, or used as the first-run mode when there are no
  saved networks
- `sta` requires at least one saved Wi-Fi network
- in `sta`, MatrixHub tries saved networks automatically one by one
- if one full cycle of saved networks fails, the device waits and retries again
  later instead of giving up permanently

## Addressing Rules

- the hostname stored in Wi-Fi settings is typically `matrixhub`
- when local name resolution works, the browser address becomes
  `matrixhub.local`
- on standard first-access setups, the local AP can also use the visible SSID
  `matrixhub.local`
- a common AP address is `192.168.4.1`
- the matrix IP screen can show a Station IP, an AP IP, or `No WiFi`

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
