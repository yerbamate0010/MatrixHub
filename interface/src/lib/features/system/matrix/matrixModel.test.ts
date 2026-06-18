import { describe, expect, it } from 'vitest';
import {
	MATRIX_EFFECT_SPEED_SCALE_CONFIG,
	MATRIX_EFFECT_SPEED_SCALES,
	MATRIX_COLOR_PRESETS,
	MATRIX_CUSTOM_ICON_PIXELS,
	MATRIX_EFFECT_CATEGORIES,
	MATRIX_EFFECT_IDS,
	MATRIX_EFFECT_MODE_MAX,
	MATRIX_EFFECT_SPEED_MAX,
	MATRIX_EFFECT_SPEED_MIN,
	fromMatrixEffectSpeedScaleValue,
	fromMatrixHexColor,
	getPreferredMatrixEffectCategory,
	getPreferredMatrixEffectSpeedScale,
	getMatrixCustomIcons,
	matrixEffectCategoryContainsEffect,
	normalizeMatrixColor,
	normalizeMatrixCustomIcons,
	normalizeMatrixEffectSpeedForScale,
	toMatrixEffectSpeedScaleValue,
	toMatrixHexColor
} from './matrixModel';

describe('matrixModel', () => {
	it('converts matrix colors between numeric and hex formats', () => {
		expect(toMatrixHexColor(0x00ff00)).toBe('#00ff00');
		expect(toMatrixHexColor(255)).toBe('#0000ff');
		expect(toMatrixHexColor(0x1abcdef)).toBe('#abcdef');
		expect(fromMatrixHexColor('#00FF00')).toBe(0x00ff00);
		expect(fromMatrixHexColor('#1ABCDEF')).toBe(0xabcdef);
		expect(fromMatrixHexColor('invalid')).toBe(0);
		expect(normalizeMatrixColor(0x1abcdef)).toBe(0xabcdef);
	});

	it('contains one compact matrix effect range', () => {
		expect(MATRIX_EFFECT_MODE_MAX).toBe(69);
		expect(MATRIX_EFFECT_IDS).toEqual(Array.from({ length: 70 }, (_, effectId) => effectId));
	});

	it('keeps effect categories and color presets in the model layer', () => {
		expect(MATRIX_EFFECT_CATEGORIES.at(-1)).toEqual({
			value: 'all',
			effectIds: MATRIX_EFFECT_IDS
		});
		expect(matrixEffectCategoryContainsEffect('recommended', 11)).toBe(true);
		expect(matrixEffectCategoryContainsEffect('calm', 69)).toBe(false);
		expect(getPreferredMatrixEffectCategory(11)).toBe('recommended');
		expect(getPreferredMatrixEffectCategory(69)).toBe('dynamic');
		expect(MATRIX_COLOR_PRESETS.map((preset) => preset.id)).toEqual([
			'alert',
			'forest',
			'ocean',
			'sunset',
			'neon',
			'aurora'
		]);
	});

	it('keeps the matrix effect speed range aligned with firmware', () => {
		expect(MATRIX_EFFECT_SPEED_MIN).toBe(50);
		expect(MATRIX_EFFECT_SPEED_MAX).toBe(24 * 60 * 60 * 1000);
		expect(MATRIX_EFFECT_SPEED_SCALES).toEqual(['ms', 's', 'm', 'h']);
		expect(MATRIX_EFFECT_SPEED_SCALE_CONFIG.ms).toEqual({
			min: 50,
			max: 5000,
			step: 50,
			unitMs: 1,
			suffix: 'ms'
		});
	});

	it('normalizes missing custom icon slots', () => {
		expect(getMatrixCustomIcons()).toEqual([[], [], []]);
		expect(getMatrixCustomIcons([[1], [2], [3]])).toEqual([[], [], []]);
	});

	it('normalizes custom icons to three RGB888 slots', () => {
		const validIcon = Array.from({ length: MATRIX_CUSTOM_ICON_PIXELS }, (_, index) =>
			index === 0 ? 0x1abcdef : index
		);

		expect(normalizeMatrixCustomIcons([validIcon, [1], validIcon, validIcon])).toEqual([
			[0xabcdef, ...Array.from({ length: MATRIX_CUSTOM_ICON_PIXELS - 1 }, (_, index) => index + 1)],
			[],
			[0xabcdef, ...Array.from({ length: MATRIX_CUSTOM_ICON_PIXELS - 1 }, (_, index) => index + 1)]
		]);
	});

	it('maps effect speed values across all UI scales', () => {
		expect(getPreferredMatrixEffectSpeedScale(500)).toBe('ms');
		expect(getPreferredMatrixEffectSpeedScale(45_000)).toBe('s');
		expect(getPreferredMatrixEffectSpeedScale(90_000)).toBe('m');
		expect(getPreferredMatrixEffectSpeedScale(5 * 60 * 60 * 1000)).toBe('h');

		expect(toMatrixEffectSpeedScaleValue(1500, 's')).toBe(2);
		expect(fromMatrixEffectSpeedScaleValue(3, 'm')).toBe(180_000);
		expect(fromMatrixEffectSpeedScaleValue(3, 'h')).toBe(10_800_000);
		expect(normalizeMatrixEffectSpeedForScale(30_000, 'm')).toBe(60_000);
	});
});
