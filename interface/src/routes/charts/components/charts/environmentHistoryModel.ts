export type EnvironmentSensorKey = 'co2' | 'temperature' | 'humidity';

export interface EnvironmentSeriesInput {
	key: EnvironmentSensorKey;
	label: string;
	color: string;
	unit: string;
	decimals: number;
	data: (number | null)[];
	minTrendRange: number;
}

export interface EnvironmentSeriesStats {
	min: number;
	max: number;
	avg: number;
	first: number;
	latest: number;
	latestIndex: number;
	delta: number;
	range: number;
}

export interface EnvironmentSeriesModel extends EnvironmentSeriesInput {
	normalizedData: (number | null)[];
	stats: EnvironmentSeriesStats | null;
}

export interface EnvironmentSampleValue {
	key: EnvironmentSensorKey;
	label: string;
	color: string;
	unit: string;
	decimals: number;
	value: number | null;
}

const TREND_MIDPOINT = 50;
const TREND_VISIBLE_SPAN = 72;
const TREND_MIN = 6;
const TREND_MAX = 94;

function isFiniteNumber(value: number | null | undefined): value is number {
	return typeof value === 'number' && Number.isFinite(value);
}

function clamp(value: number, min: number, max: number) {
	return Math.min(max, Math.max(min, value));
}

export function calculateEnvironmentSeriesStats(
	data: (number | null)[]
): EnvironmentSeriesStats | null {
	const valid: { value: number; index: number }[] = [];

	data.forEach((value, index) => {
		if (isFiniteNumber(value)) {
			valid.push({ value, index });
		}
	});

	if (valid.length === 0) return null;

	const values = valid.map((entry) => entry.value);
	const min = Math.min(...values);
	const max = Math.max(...values);
	const first = valid[0].value;
	const latestEntry = valid[valid.length - 1];
	const latest = latestEntry.value;
	const avg = values.reduce((sum, value) => sum + value, 0) / values.length;

	return {
		min,
		max,
		avg,
		first,
		latest,
		latestIndex: latestEntry.index,
		delta: latest - first,
		range: max - min
	};
}

export function normalizeEnvironmentTrend(
	data: (number | null)[],
	minTrendRange: number
): (number | null)[] {
	const stats = calculateEnvironmentSeriesStats(data);
	if (!stats) return data.map(() => null);

	const trendRange = Math.max(stats.range, minTrendRange);
	const center = (stats.min + stats.max) / 2;

	return data.map((value) => {
		if (!isFiniteNumber(value)) return null;
		const normalized = TREND_MIDPOINT + ((value - center) / trendRange) * TREND_VISIBLE_SPAN;
		return clamp(normalized, TREND_MIN, TREND_MAX);
	});
}

export function buildEnvironmentSeriesModels(
	series: EnvironmentSeriesInput[]
): EnvironmentSeriesModel[] {
	return series.map((input) => ({
		...input,
		stats: calculateEnvironmentSeriesStats(input.data),
		normalizedData: normalizeEnvironmentTrend(input.data, input.minTrendRange)
	}));
}

export function getEnvironmentSampleValues(
	series: EnvironmentSeriesModel[],
	index: number | null
): EnvironmentSampleValue[] {
	return series.map(({ key, label, color, unit, decimals, data }) => ({
		key,
		label,
		color,
		unit,
		decimals,
		value: index == null ? null : (data[index] ?? null)
	}));
}

export function formatSensorValue(
	value: number | null | undefined,
	decimals: number,
	unit: string
): string {
	if (!isFiniteNumber(value)) return '-';
	return `${value.toFixed(decimals)}${unit}`;
}
