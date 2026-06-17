# Frontend UX Release Notes

This note summarizes the user-visible changes from the MatrixHub frontend UX
refactor.

## Matrix LED

- Matrix LED Settings and Visual Effects now use the same draft flow: change a
  value, then click `Save` on that card to write it to the device.
- Each editable Matrix LED card has its own `Save` and `Discard` actions, so the
  next step is visible next to the settings being changed.
- Selecting an idle effect no longer saves immediately. Effect collection, mode,
  speed, palettes, and colors are saved together through the Visual Effects card.
- The alarm icon editor stages icon changes on the page. The Matrix LED Settings
  card `Save` writes those icons to the device.
- The button menu setting is now presented as a real setting. Keep it enabled
  when WiFi mode recovery should remain available from the physical menu.
- Leaving a page with unsaved settings asks for confirmation before the draft is
  lost.

## Interface

- Cards are calmer and less decorative, with settings and data kept in the
  foreground.
- Repeated helper text was reduced where the control label already explains the
  action.
- Disabled controls now use real disabled states instead of pointer-only blocks,
  which improves keyboard and screen-reader behavior.
- Frontend loading now records native performance marks for boot, auth, shell,
  first status, and first interactive timing.

## Quality

- A single frontend quality command now checks formatting, Svelte diagnostics,
  unit tests, unused code, dependency boundaries, production build, and size
  budgets.
- Playwright covers the Matrix LED save flow, unsaved-change guard,
  accessibility smoke checks, and responsive overflow checks.
