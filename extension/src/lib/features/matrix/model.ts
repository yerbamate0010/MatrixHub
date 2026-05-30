import type { MatrixSettings } from "@matrixhub/device-sdk";
import type { I18nRuntime } from "$lib/i18n/runtime";

export interface MatrixSettingsDraft {
  brightness: number;
  effectEnabled: boolean;
  menuScrollSpeed: number;
}

export const DEFAULT_MATRIX_SETTINGS: MatrixSettings = {
  brightness: 20,
  alarm_mode: 1,
  rotation: 0,
  auto_rotate: false,
  effect_enabled: false,
  effect_mode: 0,
  effect_speed: 1000,
  effect_color: 0x00ff00,
  effect_color_2: 0xff0000,
  effect_color_3: 0x0000ff,
  menu_enabled: true,
  menu_text_color: 0xffffff,
  menu_scroll_speed: 20,
};

export const MATRIX_ALARM_MODE_OPTIONS = [
  {
    value: 0,
    key: "solid",
  },
  {
    value: 1,
    key: "icon",
  },
  {
    value: 2,
    key: "scroll",
  },
] as const;

function clampInteger(value: number, min: number, max: number) {
  if (!Number.isFinite(value)) {
    return min;
  }

  return Math.min(max, Math.max(min, Math.round(value)));
}

export function createMatrixSettingsDraft(
  settings: MatrixSettings | null,
): MatrixSettingsDraft {
  const base = settings ?? DEFAULT_MATRIX_SETTINGS;

  return {
    brightness: clampInteger(base.brightness, 0, 255),
    effectEnabled: base.effect_enabled,
    menuScrollSpeed: clampInteger(base.menu_scroll_speed, 20, 120),
  };
}

export function buildMatrixSettingsPatch(
  draft: MatrixSettingsDraft,
): Partial<MatrixSettings> {
  return {
    brightness: clampInteger(draft.brightness, 0, 255),
    effect_enabled: draft.effectEnabled,
    menu_scroll_speed: clampInteger(draft.menuScrollSpeed, 20, 120),
  };
}

export function formatMatrixMenuState(enabled: boolean, i18n: I18nRuntime) {
  return i18n.t(enabled ? "matrix.state.enabled" : "matrix.state.disabled");
}

export function formatMatrixEffectState(enabled: boolean, i18n: I18nRuntime) {
  return i18n.t(enabled ? "matrix.state.enabled" : "matrix.state.disabled");
}

export function describeMatrixAlarmMode(value: number) {
  return (
    MATRIX_ALARM_MODE_OPTIONS.find((option) => option.value === value)?.key ??
    null
  );
}

export function formatMatrixAlarmMode(value: number, i18n: I18nRuntime) {
  const key = describeMatrixAlarmMode(value);
  return key ? i18n.t(`matrix.mode.${key}`) : i18n.t("matrix.mode.unknown");
}
