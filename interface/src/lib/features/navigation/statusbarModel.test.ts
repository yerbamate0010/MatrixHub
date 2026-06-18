import { describe, expect, it } from 'vitest';
import {
	createStatusbarClock,
	getStatusbarConnectionClass,
	getStatusbarTempColor,
	resolveStatusbarTitle
} from './statusbarModel';
import type { MenuItem } from './menuConfig';

describe('statusbarModel', () => {
	it('formats clock from device timestamp plus local drift', () => {
		const expected = new Date(Date.UTC(2026, 2, 18, 9, 6, 0));
		const result = createStatusbarClock(
			{
				timestamp: Date.UTC(2026, 2, 18, 9, 5, 0),
				lastUpdate: 1_000
			},
			61_000
		);

		expect(result).toEqual({
			date: '18.03.2026',
			clock: `${String(expected.getHours()).padStart(2, '0')}:${String(expected.getMinutes()).padStart(2, '0')}`
		});
	});

	it('falls back to browser time when device timestamp is invalid', () => {
		const fallbackDate = new Date(Date.UTC(2026, 2, 18, 12, 34, 0));
		const result = createStatusbarClock(
			{
				timestamp: 0,
				lastUpdate: 0
			},
			Date.UTC(2026, 2, 18, 12, 34, 0)
		);

		expect(result).toEqual({
			date: '18.03.2026',
			clock: `${String(fallbackDate.getHours()).padStart(2, '0')}:${String(fallbackDate.getMinutes()).padStart(2, '0')}`
		});
	});

	it('resolves nested menu titles and falls back cleanly', () => {
		const menu = [
			{
				title: 'Top',
				icon: {} as MenuItem['icon'],
				feature: true,
				submenu: [
					{
						title: 'Nested',
						icon: {} as MenuItem['icon'],
						href: '/nested',
						feature: true
					}
				]
			}
		] satisfies MenuItem[];

		expect(resolveStatusbarTitle(menu, '/nested', 'Fallback')).toBe('Nested');
		expect(resolveStatusbarTitle(menu, '/missing', 'Fallback')).toBe('Fallback');
	});

	it('maps temperature to adaptive color-mix style', () => {
		expect(getStatusbarTempColor(40)).toContain('hsl(140');
		expect(getStatusbarTempColor(90)).toContain('hsl(0');
	});

	it('maps live connection statuses to stable utility classes', () => {
		expect(getStatusbarConnectionClass('connected')).toBe('text-success');
		expect(getStatusbarConnectionClass('connecting')).toBe('text-warning');
		expect(getStatusbarConnectionClass('disconnected')).toBe('text-error');
		expect(getStatusbarConnectionClass('error')).toBe('text-error');
	});
});
