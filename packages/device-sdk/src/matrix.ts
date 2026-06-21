import {
  createDeviceApiClient,
  type DeviceApiClientOptions,
} from "./api/client";

export interface MatrixSettings {
  brightness: number;
  alarm_mode: number;
  rotation: number;
  auto_rotate: boolean;
  effect_enabled: boolean;
  effect_engine: number;
  effect_mode: number;
  effect_speed: number;
  effect_color: number;
  effect_color_2: number;
  effect_color_3: number;
  effect_reactivity_provider: number;
  effect_reactivity_gain: number;
  custom_icons?: number[][];
  menu_enabled: boolean;
  menu_text_color: number;
  menu_scroll_speed: number;
}

const MATRIX_COLOR_MASK = 0xffffff;
const MATRIX_CUSTOM_ICON_SLOTS = 3;
const MATRIX_CUSTOM_ICON_PIXELS = 64;
const MATRIX_EFFECT_ENGINE_MAX = 1;
const MATRIX_EFFECT_MODE_MAX = 69;
const MATRIX_NATIVE_3D_EFFECT_MODE_MAX = 3;
const MATRIX_EFFECT_REACTIVITY_PROVIDER_MAX = 1;
const MATRIX_EFFECT_REACTIVITY_GAIN_MAX = 200;
const MATRIX_EFFECT_SPEED_MIN = 50;
const MATRIX_EFFECT_SPEED_MAX = 24 * 60 * 60 * 1000;

function isObject(value: unknown): value is Record<string, unknown> {
  return !!value && typeof value === "object";
}

function optionalNumber(value: unknown) {
  return typeof value === "number" && Number.isFinite(value)
    ? value
    : undefined;
}

function optionalBoolean(value: unknown) {
  return typeof value === "boolean" ? value : undefined;
}

function clampInteger(value: number, min: number, max: number) {
  return Math.min(max, Math.max(min, Math.round(value)));
}

function normalizeColor(value: number) {
  return Math.max(0, Math.round(value)) & MATRIX_COLOR_MASK;
}

function sanitizeCustomIcons(value: unknown): number[][] | null | undefined {
  if (!Array.isArray(value)) {
    return undefined;
  }

  if (value.length !== MATRIX_CUSTOM_ICON_SLOTS) {
    return null;
  }

  const icons: number[][] = [];
  for (const entry of value) {
    if (!Array.isArray(entry)) {
      return null;
    }

    if (entry.length === 0) {
      icons.push([]);
      continue;
    }

    if (entry.length !== MATRIX_CUSTOM_ICON_PIXELS) {
      return null;
    }

    const pixels: number[] = [];
    for (const pixel of entry) {
      if (typeof pixel !== "number" || !Number.isFinite(pixel)) {
        return null;
      }
      pixels.push(normalizeColor(pixel));
    }
    icons.push(pixels);
  }

  return icons;
}

export function parseMatrixSettings(value: unknown): MatrixSettings | null {
  if (!isObject(value)) {
    return null;
  }

  const brightness = optionalNumber(value.brightness);
  const alarmMode = optionalNumber(value.alarm_mode);
  const rotation = optionalNumber(value.rotation);
  const autoRotate = optionalBoolean(value.auto_rotate);
  const effectEnabled = optionalBoolean(value.effect_enabled);
  const effectEngine = optionalNumber(value.effect_engine);
  const effectMode = optionalNumber(value.effect_mode);
  const effectSpeed = optionalNumber(value.effect_speed);
  const effectColor = optionalNumber(value.effect_color);
  const effectColor2 = optionalNumber(value.effect_color_2);
  const effectColor3 = optionalNumber(value.effect_color_3);
  const effectReactivityProvider = optionalNumber(value.effect_reactivity_provider);
  const effectReactivityGain = optionalNumber(value.effect_reactivity_gain);
  const menuEnabled = optionalBoolean(value.menu_enabled);
  const menuTextColor = optionalNumber(value.menu_text_color);
  const menuScrollSpeed = optionalNumber(value.menu_scroll_speed);

  if (
    brightness === undefined ||
    alarmMode === undefined ||
    rotation === undefined ||
    autoRotate === undefined ||
    effectEnabled === undefined ||
    effectEngine === undefined ||
    effectMode === undefined ||
    effectSpeed === undefined ||
    effectColor === undefined ||
    effectColor2 === undefined ||
    effectColor3 === undefined ||
    effectReactivityProvider === undefined ||
    effectReactivityGain === undefined ||
    menuEnabled === undefined ||
    menuTextColor === undefined ||
    menuScrollSpeed === undefined
  ) {
    return null;
  }

  const normalizedEffectEngine = clampInteger(
    effectEngine,
    0,
    MATRIX_EFFECT_ENGINE_MAX,
  );
  const effectModeMax =
    normalizedEffectEngine === 1
      ? MATRIX_NATIVE_3D_EFFECT_MODE_MAX
      : MATRIX_EFFECT_MODE_MAX;

  const settings: MatrixSettings = {
    brightness: clampInteger(brightness, 0, 255),
    alarm_mode: clampInteger(alarmMode, 0, 2),
    rotation: clampInteger(rotation, 0, 3),
    auto_rotate: autoRotate,
    effect_enabled: effectEnabled,
    effect_engine: normalizedEffectEngine,
    effect_mode: clampInteger(effectMode, 0, effectModeMax),
    effect_speed: clampInteger(
      effectSpeed,
      MATRIX_EFFECT_SPEED_MIN,
      MATRIX_EFFECT_SPEED_MAX,
    ),
    effect_color: normalizeColor(effectColor),
    effect_color_2: normalizeColor(effectColor2),
    effect_color_3: normalizeColor(effectColor3),
    effect_reactivity_provider: clampInteger(
      effectReactivityProvider,
      0,
      MATRIX_EFFECT_REACTIVITY_PROVIDER_MAX,
    ),
    effect_reactivity_gain: clampInteger(
      effectReactivityGain,
      0,
      MATRIX_EFFECT_REACTIVITY_GAIN_MAX,
    ),
    menu_enabled: menuEnabled,
    menu_text_color: normalizeColor(menuTextColor),
    menu_scroll_speed: clampInteger(menuScrollSpeed, 20, 120),
  };

  const customIcons = sanitizeCustomIcons(value.custom_icons);
  if (customIcons === null) {
    return null;
  }
  if (customIcons) {
    settings.custom_icons = customIcons;
  }

  return settings;
}

function requireParsed<T>(value: T | null, message: string) {
  if (!value) {
    throw new Error(message);
  }

  return value;
}

export class DeviceMatrixApi {
  private client;

  constructor(options: DeviceApiClientOptions) {
    this.client = createDeviceApiClient(options);
  }

  async getSettings(): Promise<MatrixSettings> {
    const response = await this.client.get<unknown>("/api/matrix/settings");
    return requireParsed(
      parseMatrixSettings(response),
      "matrix/settings_invalid",
    );
  }

  async updateSettings(
    settings: Partial<MatrixSettings>,
  ): Promise<MatrixSettings> {
    const response = await this.client.post<unknown>(
      "/api/matrix/settings",
      settings,
    );
    return requireParsed(
      parseMatrixSettings(response),
      "matrix/settings_invalid",
    );
  }
}
