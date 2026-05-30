# Shelly Devices

Navigation: [Home](../README.md) · [Basic Flows](../README.md#basic-use-cases) · [Additional Flows](../README.md#additional-use-cases) · [Reference](../README.md#reference-sections)

Shelly integration is optional and works best when MatrixHub and Shelly devices
share the same network.

Reading access is enough to view saved devices. Adding, editing, deleting, and
toggling Shelly relays requires management access.

## Empty State

If no devices are configured yet, the page starts with an empty state:

![Shelly empty state](../screenshots/shelly/shelly-empty-state.png)

## Add Device

Add a new device with `Add Device`:

![Shelly add device modal](../screenshots/shelly/shelly-add-device-modal.png)

The modal is where you define:

- `Name`: the label shown in the list and alarm selectors
- `IP Address`: the Shelly device IPv4 address on your LAN
- `Relay Index`: the relay channel MatrixHub should use
- `Generation`: whether the device is `Gen 1` or `Gen 2`

Useful details:

- MatrixHub supports up to four Shelly devices
- the IP field is normalized to an IPv4 host, so pasted `http://` URLs are
  trimmed automatically
- `Relay Index` normally starts at `0` for the first channel

## Device List

After a device is saved, it appears in the list with live status and basic
telemetry:

![Shelly device list](../screenshots/shelly/shelly-device-list.png)

The list can show:

- online or offline state
- the relay index badge
- current `ON` or `OFF` state
- device IP and RSSI
- live power, voltage, current, temperature, and energy values when available
- quick actions to toggle, edit, or delete the saved entry

This is also the same device list used when you attach Shelly actions to alarm
rules.

Use Shelly integration when you want to:

- monitor a relay or power-capable Shelly device
- keep related equipment visible from the same interface
- combine environmental monitoring with simple device context
- let alarms interact with selected Shelly relays

Navigation: [Home](../README.md) · [Basic Flows](../README.md#basic-use-cases) · [Additional Flows](../README.md#additional-use-cases) · [Reference](../README.md#reference-sections)
