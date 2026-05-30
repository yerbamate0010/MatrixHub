import { describe, expect, it } from 'vitest';
import {
	MATRIX_EFFECT_SPEED_SCALE_CONFIG,
	MATRIX_EFFECT_SPEED_SCALES,
	MATRIX_EFFECT_IDS,
	MATRIX_EFFECT_MODE_MAX,
	MATRIX_EFFECT_SPEED_MAX,
	MATRIX_EFFECT_SPEED_MIN,
	fromMatrixEffectSpeedScaleValue,
	fromMatrixHexColor,
	getPreferredMatrixEffectSpeedScale,
	getMatrixCustomIcons,
	normalizeMatrixEffectSpeedForScale,
	toMatrixEffectSpeedScaleValue,
	toMatrixHexColor
} from './matrixModel';

describe('matrixModel', () => {
	it('converts matrix colors between numeric and hex formats', () => {
		expect(toMatrixHexColor(0x00ff00)).toBe('#00ff00');
		expect(toMatrixHexColor(255)).toBe('#0000ff');
		expect(fromMatrixHexColor('#00FF00')).toBe(0x00ff00);
		expect(fromMatrixHexColor('invalid')).toBe(0);
	});

	it('contains one compact matrix effect range', () => {
		expect(MATRIX_EFFECT_MODE_MAX).toBe(69);
		expect(MATRIX_EFFECT_IDS).toEqual(Array.from({ length: 70 }, (_, effectId) => effectId));
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
		expect(getMatrixCustomIcons([[1], [2], [3]])).toEqual([[1], [2], [3]]);
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
