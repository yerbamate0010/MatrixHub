# Matrix LED

Navigation: [Home](../../README.md) · [Basic Flows](../../README.md#basic-use-cases) · [Additional Flows](../../README.md#additional-use-cases) · [Reference](../../README.md#reference-sections) · [System and maintenance](../system.md)

The `Matrix LED` page controls how the physical matrix display behaves on the
device.

This is the same frontend screen used on the `/system/matrix` route.

![Matrix LED page](../../screenshots/system/matrix-led-page.png)

Users with read access can inspect current matrix settings. Management access
is required to change them.

## Saving Changes

Editable cards use a draft model. Changing a control updates the page draft
first. Click `Save` on the card to write the draft to the device, or `Discard`
to return that card to the last saved settings.

The icon editor follows the same rule. Its modal `Save` updates the Matrix LED
page draft, then the card `Save` writes the full Matrix LED settings payload to
the device.

## Main Display Settings

The settings card covers:

- button menu enabled or disabled
- menu text color
- alarm display mode
- brightness
- scroll speed
- rotation and auto-rotate behavior

Menu text color is adjusted directly from the page:

![Matrix LED text color picker](../../screenshots/system/matrix-led-text-color-picker.png)

Alarm display mode decides how active alarms appear on the matrix. This is the
most important setting when you want the device itself to communicate status
without opening the web UI.

The `Button Menu` setting controls whether the physical on-device menu can be
opened from the matrix button. Keep it enabled when the device should remain
recoverable without the browser.

## Custom Severity Icons

The page can also open an icon editor for severity-specific alarm symbols.

![Matrix LED info icon editor](../../screenshots/system/matrix-led-info-icon-editor.png)

![Matrix LED warning icon editor](../../screenshots/system/matrix-led-warning-icon-editor.png)

![Matrix LED critical icon editor](../../screenshots/system/matrix-led-critical-icon-editor.png)

Use custom icons when you want the matrix to show a clearer visual distinction
between informational, warning, and critical alarms.

Icon changes are saved with the Matrix LED Settings card. Closing the modal
without saving leaves the current draft unchanged.

## Idle Effects

The separate effects card controls idle animation behavior:

- master effect enable switch
- effect mode
- effect speed
- up to three effect colors

![Matrix LED effects dropdown](../../screenshots/system/matrix-led-effects-dropdown.png)

These effects shape how the device looks when it is not actively showing menu
or alarm content.

Effect controls are disabled until `Enable Effects` is on. Effect collection,
mode, speed, palettes, and colors are all saved through the Visual Effects card
`Save` action. Selecting an effect no longer writes to the device immediately.

## Physical Button Menu

When the matrix menu is enabled, the physical `BOOT` button controls a compact
on-device menu:

- hold `BOOT` for about `2 seconds` outside the menu to open it
- short-press to cycle: `TIME`, `SENSORS`, `IP`, `WIFI STA`, `WIFI AP`,
  `WIFI OFF`, `EXIT`
- hold `BOOT` for about `2 seconds` while a menu item is visible to select it
- select `EXIT` to close the menu
- selecting `WIFI STA`, `WIFI AP`, or `WIFI OFF` shows `RELEASE +2x`; release
  the button and short-press twice to save the mode and restart
- the factory reset gesture remains separate: a 10 second hold followed by the
  confirmation double-click

The WiFi mode selections above are intentionally guarded by the release and
double short-press gesture because they can change how you reconnect to the
device.

## Important Behavior

- `Matrix LED` affects the device display, not the browser theme
- the page is useful both for readability and for making alerts more obvious
  from across a room
- the current settings stay inspectable even when the session cannot manage
  hardware settings
- leaving the page with unsaved Matrix LED drafts asks for confirmation before
  the draft is lost

## Related Pages

- [Alarms](../alarms.md)
- [Customize Matrix LED alerts](../../flows/additional/customize-matrix-led.md)
- [Styles](styles.md)

Navigation: [Home](../../README.md) · [Basic Flows](../../README.md#basic-use-cases) · [Additional Flows](../../README.md#additional-use-cases) · [Reference](../../README.md#reference-sections) · [System and maintenance](../system.md)
