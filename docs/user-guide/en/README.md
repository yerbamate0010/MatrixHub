# MatrixHub User Guide (EN)

Use flow guides for common tasks and reference sections for page-by-page UI
descriptions.

## Basic Use Cases

- [Get online and connect to home Wi-Fi](flows/basic/get-online-and-connect-home-wifi.md)
- [Add your first alarm rule](flows/basic/add-first-alarm.md)
- [Set up Telegram notifications](flows/basic/setup-telegram-notifications.md)

## Additional Use Cases

- [Set up Webhook notifications](flows/additional/set-up-webhook-notifications.md)
- [Set up Pushover notifications](flows/additional/set-up-pushover-notifications.md)
- [Set up Heartbeat monitor](flows/additional/set-up-heartbeat-monitor.md)
- [Set up UDP data push](flows/additional/set-up-udp-data-push.md)
- [Add a Bluetooth sensor](flows/additional/add-bluetooth-sensor.md)
- [Add a Shelly device](flows/additional/add-shelly-device.md)
- [Customize Matrix LED alerts](flows/additional/customize-matrix-led.md)
- [Download data logs](flows/additional/download-data-logs.md)

## Reference Sections

- [Overview and first access](sections/overview.md)
- [Wi-Fi configuration](sections/wifi.md)
- [Alarms](sections/alarms.md)
- [Notifications](sections/notifications.md)
- [Shelly devices](sections/shelly.md)
- [Bluetooth devices](sections/bluetooth.md)
- [USB features](sections/usb-features.md)
- [Wi-Fi sensing and CSI](sections/wifi-sensing.md) (diagnostic pages)

## System Reference Pages

- [System and maintenance hub](sections/system.md)
- [System Status](sections/system/status.md)
- [Data Logs](sections/system/data-logs.md)
- [Time](sections/system/time.md)
- [Matrix LED](sections/system/matrix-led.md)
- [Styles](sections/system/styles.md)
- [Compensation / SCD41 Tuning](sections/system/compensation.md) (admin only)
- [Power Settings](sections/system/power.md) (admin only)
- [Users](sections/system/users.md) (admin only)
- [File Manager](sections/system/file-manager.md) (admin-oriented, can be disabled)

## Special / Support Screens

- [Support appendix](appendix/README.md)
- [Behavior and availability](appendix/behavior-and-availability.md)
- [Help page](appendix/help.md)
- [Login screen](appendix/login.md)

## Notes

- Flow guides are task-oriented and meant to answer "how do I do this?"
- Reference sections are screen-oriented and meant to answer "what is on this page?"
- The support appendix captures predictable runtime rules such as Wi-Fi
  fallback and page availability
- Some advanced flows, such as a full `@BotFather` walkthrough for Telegram, can be expanded later without changing this structure.

## Video Demos

- [This Device Warns Me About High CO2 and Sends It to Telegram](https://youtu.be/uUkitUXndD8?si=6oeI0EoqvSQORf07)
- [MatrixHub Full Interface Tour: Alerts, Telegram, BLE, Shelly, and System Pages](https://youtu.be/ElIpB5tRcJQ)

## Route Coverage Notes

- `/` is documented in [Overview and first access](sections/overview.md)
- `/wifi/sta` and `/wifi/ap` are documented in [Wi-Fi configuration](sections/wifi.md)
  and [Get online and connect to home Wi-Fi](flows/basic/get-online-and-connect-home-wifi.md)
- `/settings/notifications/telegram` is documented in
  [Notifications](sections/notifications.md) and
  [Set up Telegram notifications](flows/basic/setup-telegram-notifications.md)
- `/settings/notifications/pushover` is documented in
  [Notifications](sections/notifications.md) and
  [Set up Pushover notifications](flows/additional/set-up-pushover-notifications.md)
- `/settings/notifications/webhook` is documented in
  [Notifications](sections/notifications.md) and
  [Set up Webhook notifications](flows/additional/set-up-webhook-notifications.md)
- `/settings/notifications/heartbeat` is documented in
  [Notifications](sections/notifications.md) and
  [Set up Heartbeat monitor](flows/additional/set-up-heartbeat-monitor.md)
- `/settings/integrations/udp` is documented in
  [Notifications](sections/notifications.md) and
  [Set up UDP data push](flows/additional/set-up-udp-data-push.md)
- `/connections/ntp` is documented in [Time](sections/system/time.md)
- `/logs` is documented in [Data Logs](sections/system/data-logs.md) and
  [Download data logs](flows/additional/download-data-logs.md)
- `/system/status` is documented in [System Status](sections/system/status.md)
- `/settings/sensors/compensation` is documented in
  [Compensation / SCD41 Tuning](sections/system/compensation.md)
- `/system/power` is documented in [Power Settings](sections/system/power.md)
- `/system/matrix` is documented in [Matrix LED](sections/system/matrix-led.md)
- `/user` is documented in [Users](sections/system/users.md)
- `/system/file-manager` is documented in
  [File Manager](sections/system/file-manager.md)
- `/system/styles` is documented in [Styles](sections/system/styles.md)
- `/system/help` is documented in [Help page](appendix/help.md)
- `/login` is documented in [Login screen](appendix/login.md)
- `/shelly` is documented in [Shelly devices](sections/shelly.md) and
  [Add a Shelly device](flows/additional/add-shelly-device.md)
- `/bluetooth` is documented in [Bluetooth devices](sections/bluetooth.md) and
  [Add a Bluetooth sensor](flows/additional/add-bluetooth-sensor.md)
- `/wifisensing` and `/wifisensing/csi` are documented in
  [Wi-Fi sensing and CSI](sections/wifi-sensing.md)
- `/usb-features/airmouse` is documented in
  [Air Mouse](sections/usb-features/airmouse.md)
- `/usb-features/jiggler` is documented in
  [Mouse Jiggler](sections/usb-features/mouse-jiggler.md)
- `/usb-features/keyboard` is documented in
  [Keyboard](sections/usb-features/keyboard.md)
- `/usb-features/macros` is documented in
  [Macros](sections/usb-features/macros.md)
- `/usb-features/terminal` is documented in
  [USB Terminal](sections/usb-features/usb-terminal.md)
- `/charts` is documented in [Overview and first access](sections/overview.md)
- redirect-only and shell routes do not get separate user-guide pages

[Project README](../../../README.md) · [Engineering Reference](../../engineering/README.md)
