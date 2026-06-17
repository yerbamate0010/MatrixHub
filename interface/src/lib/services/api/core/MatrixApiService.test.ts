import { beforeEach, describe, expect, it, vi } from 'vitest';

import { MatrixSettingsSchema } from '$lib/utils/validation/schemas';
import { MatrixApiService, type MatrixSettings } from './MatrixApiService';

const mockClient = {
	get: vi.fn(),
	post: vi.fn()
};

vi.mock('$lib/utils', () => ({
	createApiClient: () => mockClient
}));

function createMatrixSettings(overrides: Partial<MatrixSettings> = {}): MatrixSettings {
	return {
		brightness: 10,
		alarm_mode: 2,
		rotation: 0,
		auto_rotate: false,
		effect_enabled: false,
		effect_mode: 0,
		effect_speed: 1000,
		effect_color: 0x00ff00,
		effect_color_2: 0xff0000,
		effect_color_3: 0x0000ff,
		custom_icons: [[], [], []],
		menu_enabled: true,
		menu_text_color: 0xffffff,
		menu_scroll_speed: 60,
		...overrides
	};
}

describe('MatrixApiService', () => {
	let service: MatrixApiService;

	beforeEach(() => {
		vi.clearAllMocks();
		service = new MatrixApiService({ bearerToken: 'token' });
	});

	it('loads matrix settings', async () => {
		const settings = createMatrixSettings();
		mockClient.get.mockResolvedValue(settings);

		const result = await service.getSettings();

		expect(mockClient.get).toHaveBeenCalledWith(
			'/api/matrix/settings',
			expect.objectContaining({ schema: MatrixSettingsSchema })
		);
		expect(result).toBe(settings);
	});

	it('saves matrix settings through the canonical POST endpoint', async () => {
		const payload = createMatrixSettings({ brightness: 42 });
		mockClient.post.mockResolvedValue(payload);

		const result = await service.updateSettings(payload);

		expect(mockClient.post).toHaveBeenCalledWith(
			'/api/matrix/settings',
			payload,
			expect.objectContaining({ schema: MatrixSettingsSchema })
		);
		expect(result).toBe(payload);
	});
});
