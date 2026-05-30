import type { MatrixSettings } from '$lib/services/api/core/MatrixApiService';

export type MatrixEffectSpeedScale = 'ms' | 's' | 'm' | 'h';

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
	menu_scroll_speed: 20
};

// Keep this aligned with the backend validator in MatrixConfigJson.cpp.
export const MATRIX_EFFECT_SPEED_MIN = 50;
export const MATRIX_EFFECT_SPEED_MAX = 24 * 60 * 60 * 1000;

export const MATRIX_EFFECT_SPEED_SCALE_CONFIG = {
	ms: { min: 50, max: 5000, step: 50, unitMs: 1, suffix: 'ms' },
	s: { min: 1, max: 60, step: 1, unitMs: 1000, suffix: 's' },
	m: { min: 1, max: 180, step: 1, unitMs: 60 * 1000, suffix: 'm' },
	h: { min: 1, max: 24, step: 1, unitMs: 60 * 60 * 1000, suffix: 'h' }
} as const satisfies Record<
	MatrixEffectSpeedScale,
	{ min: number; max: number; step: number; unitMs: number; suffix: string }
>;

export const MATRIX_EFFECT_SPEED_SCALES: MatrixEffectSpeedScale[] = ['ms', 's', 'm', 'h'];

// Keep this aligned with the backend validator. Matrix effects use one compact
// 0..69 range end-to-end, with no hidden holes or excluded vendor IDs.
export const MATRIX_EFFECT_MODE_MAX = 69;

export const MATRIX_EFFECT_IDS = Array.from(
	{ length: MATRIX_EFFECT_MODE_MAX + 1 },
	(_, effectId) => effectId
);

function clampMatrixEffectSpeed(value: number): number {
	if (!Number.isFinite(value)) return DEFAULT_MATRIX_SETTINGS.effect_speed;
	return Math.min(MATRIX_EFFECT_SPEED_MAX, Math.max(MATRIX_EFFECT_SPEED_MIN, Math.round(value)));
}

function snapScaleValue(value: number, scale: MatrixEffectSpeedScale): number {
	const { min, max, step } = MATRIX_EFFECT_SPEED_SCALE_CONFIG[scale];
	const clamped = Math.min(max, Math.max(min, value));
	const snapped = min + Math.round((clamped - min) / step) * step;
	return Number(snapped.toFixed(2));
}

export function getPreferredMatrixEffectSpeedScale(effectSpeedMs: number): MatrixEffectSpeedScale {
	const normalized = clampMatrixEffectSpeed(effectSpeedMs);

	if (normalized < 1000) return 'ms';
	if (normalized < 60 * 1000) return 's';
	if (normalized < 60 * 60 * 1000) return 'm';
	return 'h';
}

export function toMatrixEffectSpeedScaleValue(
	effectSpeedMs: number,
	scale: MatrixEffectSpeedScale
): number {
	const normalized = clampMatrixEffectSpeed(effectSpeedMs);
	return snapScaleValue(normalized / MATRIX_EFFECT_SPEED_SCALE_CONFIG[scale].unitMs, scale);
}

export function fromMatrixEffectSpeedScaleValue(
	value: number,
	scale: MatrixEffectSpeedScale
): number {
	const snapped = snapScaleValue(value, scale);
	return clampMatrixEffectSpeed(snapped * MATRIX_EFFECT_SPEED_SCALE_CONFIG[scale].unitMs);
}

export function normalizeMatrixEffectSpeedForScale(
	effectSpeedMs: number,
	scale: MatrixEffectSpeedScale
): number {
	return fromMatrixEffectSpeedScaleValue(
		toMatrixEffectSpeedScaleValue(effectSpeedMs, scale),
		scale
	);
}

export function toMatrixHexColor(value: number): string {
	const normalized = Number.isFinite(value) ? Math.max(0, Math.trunc(value)) : 0;
	return `#${normalized.toString(16).padStart(6, '0').slice(-6)}`;
}

export function fromMatrixHexColor(value: string): number {
	const parsed = Number.parseInt(value.replace(/^#/, ''), 16);
	return Number.isFinite(parsed) ? parsed : 0;
}

export function getMatrixCustomIcons(value?: number[][]): number[][] {
	return value && value.length > 0 ? value : [[], [], []];
}
