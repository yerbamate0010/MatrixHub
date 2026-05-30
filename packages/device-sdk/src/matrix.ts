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
  effect_mode: number;
  effect_speed: number;
  effect_color: number;
  effect_color_2: number;
  effect_color_3: number;
  custom_icons?: number[][];
  menu_enabled: boolean;
  menu_text_color: number;
  menu_scroll_speed: number;
}

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

function sanitizeCustomIcons(value: unknown) {
  if (!Array.isArray(value)) {
    return undefined;
  }

  const icons = value
    .map((entry) => {
      if (!Array.isArray(entry)) {
        return null;
      }

      const pixels = entry.filter(
        (pixel): pixel is number =>
          typeof pixel === "number" && Number.isFinite(pixel),
      );

      return pixels.length === entry.length ? pixels : null;
    })
    .filter((entry): entry is number[] => entry !== null);

  return icons.length > 0 ? icons : undefined;
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
  const effectMode = optionalNumber(value.effect_mode);
  const effectSpeed = optionalNumber(value.effect_speed);
  const effectColor = optionalNumber(value.effect_color);
  const effectColor2 = optionalNumber(value.effect_color_2);
  const effectColor3 = optionalNumber(value.effect_color_3);
  const menuEnabled = optionalBoolean(value.menu_enabled);
  const menuTextColor = optionalNumber(value.menu_text_color);
  const menuScrollSpeed = optionalNumber(value.menu_scroll_speed);

  if (
    brightness === undefined ||
    alarmMode === undefined ||
    rotation === undefined ||
    autoRotate === undefined ||
    effectEnabled === undefined ||
    effectMode === undefined ||
    effectSpeed === undefined ||
    effectColor === undefined ||
    effectColor2 === undefined ||
    effectColor3 === undefined ||
    menuEnabled === undefined ||
    menuTextColor === undefined ||
    menuScrollSpeed === undefined
  ) {
    return null;
  }

  const settings: MatrixSettings = {
    brightness: clampInteger(brightness, 0, 255),
    alarm_mode: clampInteger(alarmMode, 0, 2),
    rotation: clampInteger(rotation, 0, 3),
    auto_rotate: autoRotate,
    effect_enabled: effectEnabled,
    effect_mode: clampInteger(effectMode, 0, 255),
    effect_speed: Math.max(0, Math.round(effectSpeed)),
    effect_color: Math.max(0, Math.round(effectColor)),
    effect_color_2: Math.max(0, Math.round(effectColor2)),
    effect_color_3: Math.max(0, Math.round(effectColor3)),
    menu_enabled: menuEnabled,
    menu_text_color: Math.max(0, Math.round(menuTextColor)),
    menu_scroll_speed: clampInteger(menuScrollSpeed, 20, 120),
  };

  const customIcons = sanitizeCustomIcons(value.custom_icons);
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
