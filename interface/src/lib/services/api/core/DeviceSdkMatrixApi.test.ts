import { describe, expect, it } from 'vitest';

import { parseMatrixSettings } from '@matrixhub/device-sdk';

function createMatrixPayload(overrides: Record<string, unknown> = {}) {
	return {
		brightness: 10,
		alarm_mode: 2,
		rotation: 0,
		auto_rotate: false,
		effect_enabled: false,
		effect_mode: 0,
		effect_speed: 1000,
		effect_color: 0x1abcdef,
		effect_color_2: 0xff0000,
		effect_color_3: 0x0000ff,
		custom_icons: [[], [], []],
		menu_enabled: true,
		menu_text_color: 0x1ffffff,
		menu_scroll_speed: 60,
		...overrides
	};
}

describe('device SDK matrix parser', () => {
	it('normalizes matrix colors to RGB888', () => {
		const parsed = parseMatrixSettings(createMatrixPayload());

		expect(parsed?.effect_color).toBe(0xabcdef);
		expect(parsed?.menu_text_color).toBe(0xffffff);
	});

	it('keeps effect mode and speed inside the firmware-supported range', () => {
		const parsed = parseMatrixSettings(createMatrixPayload({ effect_mode: 255, effect_speed: 1 }));

		expect(parsed?.effect_mode).toBe(69);
		expect(parsed?.effect_speed).toBe(50);
	});

	it('accepts exactly three custom icon slots with empty or 64-pixel payloads', () => {
		const icon = Array.from({ length: 64 }, (_, index) => (index === 0 ? 0x1abcdef : index));
		const parsed = parseMatrixSettings(createMatrixPayload({ custom_icons: [icon, [], icon] }));

		expect(parsed?.custom_icons?.[0]?.[0]).toBe(0xabcdef);
		expect(parsed?.custom_icons?.[1]).toEqual([]);
		expect(parsed?.custom_icons?.[2]?.length).toBe(64);
	});

	it('rejects malformed custom icon payloads', () => {
		expect(parseMatrixSettings(createMatrixPayload({ custom_icons: [[1], [], []] }))).toBeNull();
		expect(parseMatrixSettings(createMatrixPayload({ custom_icons: [[], []] }))).toBeNull();
		expect(
			parseMatrixSettings(createMatrixPayload({ custom_icons: [[Number.NaN], [], []] }))
		).toBeNull();
	});
});
