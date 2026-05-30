# Wi-Fi Configuration

Navigation: [Home](../README.md) · [Basic Flows](../README.md#basic-use-cases) · [Additional Flows](../README.md#additional-use-cases) · [Reference](../README.md#reference-sections)

This section covers the pages used to manage network access, fallback access,
and recovery behavior.

The same `WiFi` menu area also contains [Shelly devices](shelly.md),
[Wi-Fi sensing and CSI](wifi-sensing.md), and the related `WiFi CSI` live page.
Those features are documented separately so this page can stay focused on
network access itself.

## WiFi Station

The most important network page is `WiFi Station`. Use it to manage the
regular network connection after the initial setup.

![Wi-Fi station page](../screenshots/wifi/wifi-station-page.png)

The page combines live connection status, connection settings, and the saved
network list.

![Connection settings: mDNS and STA mode](../screenshots/wifi/wifi-station-connection-settings-panel.png)

![Saved networks actions](../screenshots/wifi/wifi-station-saved-networks-actions.png)

On this page you can:

- review the current Wi-Fi status
- check the assigned IP address
- manage saved networks
- control how the station mode reconnects
- set the hostname used for local name resolution
- configure optional static IP settings per saved network

## Access Point

The `Access Point` page controls the local fallback network used during the
initial setup or recovery.

![Access Point page](../screenshots/wifi/wifi-access-point-page.png)

Typical use cases for the Access Point page:

- restoring local access when the device cannot join the normal Wi-Fi network
- checking the AP IP address and SSID
- changing local AP parameters for service or installation work

These settings matter both during first access and later recovery situations
when MatrixHub exposes the local AP again.

## Connectivity Behavior

MatrixHub can appear in three practical connectivity states:

- `Manual AP-only`: the local AP stays up and Station recovery is intentionally
  stopped
- normal `STA`: the device joins one of the saved Wi-Fi networks
- `Rescue AP+STA`: a local AP is exposed while Station recovery continues in
  the background

Important behavior:

- if `Station (STA) Mode` is set to `Offline`, MatrixHub returns to
  `Manual AP-only`
- if no saved networks exist, MatrixHub also stays in `Manual AP-only`
- if saved networks exist, MatrixHub tries them automatically one by one
- if one full cycle fails, the device waits and retries again instead of giving
  up permanently
- during a longer outage, MatrixHub can expose a rescue AP so local recovery
  remains possible while Station mode is still retrying
- once the Station connection becomes stable again, the rescue AP is removed
  and normal Station operation resumes

## Recovery and Addressing

The hostname stored in Wi-Fi settings is the short name, usually `matrixhub`.
When local name resolution works, the browser address becomes
`matrixhub.local`.

Some standard builds also use `matrixhub.local` as the visible fallback AP
name, so the same string can appear both as a Wi-Fi network name and as a
browser address.

During setup or recovery you may therefore see:

- a current router-assigned Station IP such as `192.168.x.x`
- an AP fallback address such as `192.168.4.1`
- `No WiFi` on the matrix IP screen when nothing is currently connected

For a compact summary of these rules, see
[Behavior and availability](../appendix/behavior-and-availability.md).

## Related Areas Under the WiFi Menu

- [Shelly devices](shelly.md) for LAN relay and telemetry integration
- [Wi-Fi sensing and CSI](wifi-sensing.md) for advanced signal-based diagnostic
  pages
- [Get online and connect to home Wi-Fi](../flows/basic/get-online-and-connect-home-wifi.md)

Navigation: [Home](../README.md) · [Basic Flows](../README.md#basic-use-cases) · [Additional Flows](../README.md#additional-use-cases) · [Reference](../README.md#reference-sections)
