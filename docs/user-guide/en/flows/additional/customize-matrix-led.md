# Customize Matrix LED Alerts

Navigation: [Home](../../README.md) · [Basic Flows](../../README.md#basic-use-cases) · [Additional Flows](../../README.md#additional-use-cases) · [Reference](../../README.md#reference-sections)

Use this flow to adjust how local LED alerts look on the device.

## Goal

- choose the alarm display mode
- adjust brightness or effects
- customize severity icons if needed

## Before You Start

- sign in with an account that can manage system settings
- keep the device nearby so you can see the matrix changes on real hardware
- start with simple visual tweaks before editing all severity icons at once

## Step 1: Open the Matrix LED page

Open `System -> Matrix LED`.

The main page groups alert display mode, brightness, rotation, idle behavior,
and icon customization:

![Matrix LED page](../../screenshots/system/matrix-led-page.png)

## Step 2: Choose the main alert style

On the main page, review:

- the alarm display mode
- brightness
- rotation
- any other top-level visual options available on your build

This is the best place to make the first broad change before editing details.

## Step 3: Adjust idle look and menu readability

If you want a different idle look, open the effects dropdown:

![Matrix LED effects dropdown](../../screenshots/system/matrix-led-effects-dropdown.png)

If you want the menu text to be easier to read, use the text color picker:

![Matrix LED text color picker](../../screenshots/system/matrix-led-text-color-picker.png)

## Step 4: Customize severity icons

Open the icon editor for the severity level you want to change.

Info severity:

![Matrix LED info icon editor](../../screenshots/system/matrix-led-info-icon-editor.png)

Warning severity:

![Matrix LED warning icon editor](../../screenshots/system/matrix-led-warning-icon-editor.png)

Critical severity:

![Matrix LED critical icon editor](../../screenshots/system/matrix-led-critical-icon-editor.png)

Use this when you want the device to show your own icon language instead of the
default shapes.

## Important

- these changes affect local device visuals, not notification delivery logic
- severity icons should stay easy to recognize from a distance
- after changing brightness, rotation, or effects, verify the result on the
  physical matrix before moving on
- keeping `Info`, `Warning`, and `Critical` visually distinct makes daily alarm
  triage easier

## Related Reference Sections

- [Matrix LED](../../sections/system/matrix-led.md)
- [Alarms](../../sections/alarms.md)

Navigation: [Home](../../README.md) · [Basic Flows](../../README.md#basic-use-cases) · [Additional Flows](../../README.md#additional-use-cases) · [Reference](../../README.md#reference-sections)
