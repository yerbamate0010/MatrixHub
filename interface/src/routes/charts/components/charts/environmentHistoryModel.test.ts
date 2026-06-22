import { describe, expect, it } from 'vitest';
import {
	buildEnvironmentSeriesModels,
	calculateEnvironmentSeriesStats,
	formatSensorValue,
	getEnvironmentSampleValues,
	normalizeEnvironmentTrend
} from './environmentHistoryModel';

describe('environmentHistoryModel', () => {
	it('keeps a flat series centered on the trend scale', () => {
		const normalized = normalizeEnvironmentTrend([45.5, 45.5, 45.5], 10);

		expect(normalized).toEqual([50, 50, 50]);
	});

	it('does not amplify tiny changes to the full trend range', () => {
		const normalized = normalizeEnvironmentTrend([45.5, 45.6], 10);

		expect(normalized[0]).toBeCloseTo(49.64, 2);
		expect(normalized[1]).toBeCloseTo(50.36, 2);
	});

	it('preserves missing samples while normalizing valid values', () => {
		const normalized = normalizeEnvironmentTrend([600, null, 700], 200);

		expect(normalized[0]).toBeLessThan(50);
		expect(normalized[1]).toBeNull();
		expect(normalized[2]).toBeGreaterThan(50);
	});

	it('calculates stats from valid samples and uses the last valid value as latest', () => {
		const stats = calculateEnvironmentSeriesStats([null, 10, 12, null, 15]);

		expect(stats).toMatchObject({
			min: 10,
			max: 15,
			avg: 12.333333333333334,
			first: 10,
			latest: 15,
			latestIndex: 4,
			delta: 5,
			range: 5
		});
	});

	it('builds display models and sample values for all environment series', () => {
		const models = buildEnvironmentSeriesModels([
			{
				key: 'co2',
				label: 'CO2',
				color: '#22c55e',
				unit: 'ppm',
				decimals: 0,
				data: [600, 620],
				minTrendRange: 200
			},
			{
				key: 'temperature',
				label: 'Temperature',
				color: '#ef4444',
				unit: '°C',
				decimals: 1,
				data: [22.1, 22.6],
				minTrendRange: 5
			}
		]);

		const samples = getEnvironmentSampleValues(models, 1);

		expect(models).toHaveLength(2);
		expect(samples).toEqual([
			expect.objectContaining({ key: 'co2', value: 620 }),
			expect.objectContaining({ key: 'temperature', value: 22.6 })
		]);
	});

	it('formats missing and present sensor values consistently', () => {
		expect(formatSensorValue(612, 0, 'ppm')).toBe('612ppm');
		expect(formatSensorValue(22.456, 1, '°C')).toBe('22.5°C');
		expect(formatSensorValue(null, 1, '%')).toBe('-');
	});
});
