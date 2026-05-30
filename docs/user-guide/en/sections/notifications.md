# Notifications

Navigation: [Home](../README.md) · [Basic Flows](../README.md#basic-use-cases) · [Additional Flows](../README.md#additional-use-cases) · [Reference](../README.md#reference-sections)

MatrixHub supports local alerts and several remote notification channels. For a
first installation, this is the safest order:

Admin only: notification configuration pages require management access on the
live device.

1. Make sure Wi-Fi access works normally.
2. Keep the first alarm on `LED`.
3. Add one remote notification channel.
4. Send a test message before relying on it.

`LED`, `Telegram`, `Webhook`, and `Pushover` can be selected directly inside
alarm rules. One rule can use more than one of those actions at once.
`Heartbeat` and `UDP Data Push` work independently from alarms, while Shelly
actions are attached separately inside the alarm editor.

Use this page as a quick channel reference. For first-time setup, open the
matching flow guide instead of relying on this page alone.

Common flow guides:

- [Set up Telegram notifications](../flows/basic/setup-telegram-notifications.md)
- [Set up Webhook notifications](../flows/additional/set-up-webhook-notifications.md)
- [Set up Pushover notifications](../flows/additional/set-up-pushover-notifications.md)
- [Set up Heartbeat monitor](../flows/additional/set-up-heartbeat-monitor.md)
- [Set up UDP data push](../flows/additional/set-up-udp-data-push.md)

## Telegram Notifications

Telegram is useful when you want alarm messages in a chat that you can monitor
from your phone or share with other users.

![Telegram notifications](../screenshots/notifications/telegram-notifications-page.png)

Use the page to:

- enable Telegram delivery
- save the bot token
- auto-discover the target chat ID after the first message to the bot
- optionally enable Telegram commands
- send a test message before enabling alarms

Flow guide:

- [Set up Telegram notifications](../flows/basic/setup-telegram-notifications.md)

## Pushover Notifications

Pushover is a good option when you want fast mobile push notifications without
maintaining your own webhook receiver.

![Pushover notifications](../screenshots/notifications/pushover-notifications-page.png)

Use the page to enter the `User Key`, save the application `API Token`, and
send a test push before you attach `Pushover` to alarm rules.

Flow guide:

- [Set up Pushover notifications](../flows/additional/set-up-pushover-notifications.md)

## Heartbeat Monitor

Heartbeat monitoring is different from alarm delivery. It is useful when you
want confirmation that the device is still online and checking in regularly.

![Heartbeat monitor](../screenshots/notifications/heartbeat-monitor-page.png)

Use this page when you want a watchdog-style ping in addition to normal
threshold alarms.

Flow guide:

- [Set up Heartbeat monitor](../flows/additional/set-up-heartbeat-monitor.md)

## UDP Data Push

UDP push is intended for local collectors, dashboards, or other systems that
accept sensor data packets on the network.

![UDP data push](../screenshots/notifications/udp-data-push-page.png)

Use the page to set destination host or IP, destination port, data format, and
send interval. `Send Test Packet` helps confirm that your local receiver can see
the traffic.

Flow guide:

- [Set up UDP data push](../flows/additional/set-up-udp-data-push.md)

## Webhook Notifications

Webhook notifications send an HTTP request to an external service whenever an
alarm is triggered or when you send a test event.

![Webhook notifications](../screenshots/notifications/webhook-notifications-page.png)

This is useful for integrations such as:

- Discord
- home automation services
- custom backend systems

Discord webhooks are supported directly. When MatrixHub detects a Discord
webhook URL, it automatically adjusts the text payload to the `content` format
expected by Discord.

Treat the webhook URL as a secret. Even if the screenshot only shows part of
the address, your real production endpoint should not be published in full.

Use this page to save the webhook URL, choose the request details supported by
the page, and send a test event before enabling alarms.

Flow guide:

- [Set up Webhook notifications](../flows/additional/set-up-webhook-notifications.md)

Navigation: [Home](../README.md) · [Basic Flows](../README.md#basic-use-cases) · [Additional Flows](../README.md#additional-use-cases) · [Reference](../README.md#reference-sections)
