import { beforeEach, describe, expect, it, vi } from 'vitest';
import type { MatrixSettings } from '$lib/services/api/core/MatrixApiService';

const { mockNotifications } = vi.hoisted(() => ({
	mockNotifications: {
		success: vi.fn(),
		error: vi.fn(),
		warning: vi.fn(),
		info: vi.fn()
	}
}));

vi.mock('$lib/components/toasts/notifications.svelte', () => ({
	notifications: mockNotifications
}));

vi.mock('$lib/i18n.svelte', () => ({
	i18n: {
		languageTag: 'en'
	}
}));

vi.mock('$lib/utils', () => ({
	getRequestAbortKind: vi.fn(() => null),
	toUserRequestErrorMessage: vi.fn((error: unknown, options?: { fallbackMessage?: string }) => {
		if (error instanceof Error && error.message) return error.message;
		return options?.fallbackMessage ?? 'unknown';
	})
}));

vi.mock('$lib/paraglide/messages.js', () => ({
	matrix_err_load: () => 'matrix load failed',
	matrix_err_save: () => 'matrix save failed',
	toast_settings_updated: () => 'matrix updated',
	settings_load_error: () => 'load error',
	settings_save_error: () => 'save error',
	settings_saved: () => 'saved',
	settings_validation_error: () => 'validation error',
	toast_message: ({ message }: { message: string }) => message
}));

function createMatrixSettings(overrides: Partial<MatrixSettings> = {}): MatrixSettings {
	return {
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
		menu_scroll_speed: 20,
		...overrides
	};
}

function flushPromises() {
	return new Promise((resolve) => setTimeout(resolve, 0));
}

describe('useMatrixSettings', () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it('does not autoload settings before loadSettings is called', async () => {
		const { useMatrixSettings } = await import('./useMatrixSettings.svelte');
		const api = {
			getSettings: vi.fn().mockResolvedValue(createMatrixSettings({ brightness: 55 })),
			updateSettings: vi.fn()
		};

		let cleanup: (() => void) | undefined;

		await new Promise<void>((resolve) => {
			cleanup = $effect.root(() => {
				const matrix = useMatrixSettings(() => api as never);

				expect(api.getSettings).not.toHaveBeenCalled();

				void matrix.loadSettings().then(() => {
					expect(api.getSettings).toHaveBeenCalledOnce();
					expect(matrix.loading).toBe(false);
					expect(matrix.settings.brightness).toBe(55);
					resolve();
				});
			});
		});

		cleanup?.();
	});

	it('autoloads settings once per shouldLoad cycle', async () => {
		const { useMatrixSettings } = await import('./useMatrixSettings.svelte');
		const api = {
			getSettings: vi.fn().mockResolvedValue(createMatrixSettings({ brightness: 55 })),
			updateSettings: vi.fn()
		};

		let cleanup: (() => void) | undefined;
		let setCanLoad!: (value: boolean) => void;

		cleanup = $effect.root(() => {
			let canLoad = $state(false);
			setCanLoad = (value: boolean) => {
				canLoad = value;
			};

			useMatrixSettings(() => api as never, {
				shouldLoad: () => canLoad
			});
		});

		expect(api.getSettings).not.toHaveBeenCalled();

		setCanLoad(true);
		await vi.waitFor(() => {
			expect(api.getSettings).toHaveBeenCalledTimes(1);
		});

		await flushPromises();
		expect(api.getSettings).toHaveBeenCalledTimes(1);

		setCanLoad(false);
		await flushPromises();
		setCanLoad(true);

		await vi.waitFor(() => {
			expect(api.getSettings).toHaveBeenCalledTimes(2);
		});

		cleanup?.();
	});

	it('saves updated settings and clears the dirty flag', async () => {
		const { useMatrixSettings } = await import('./useMatrixSettings.svelte');
		const api = {
			getSettings: vi.fn().mockResolvedValue(createMatrixSettings()),
			updateSettings: vi.fn().mockResolvedValue(createMatrixSettings({ brightness: 77 }))
		};

		let cleanup: (() => void) | undefined;

		await new Promise<void>((resolve) => {
			cleanup = $effect.root(() => {
				const matrix = useMatrixSettings(() => api as never);

				void matrix.loadSettings().then(async () => {
					matrix.updateSetting('brightness', 77);
					expect(matrix.hasChanges).toBe(true);

					matrix.saveSettings();
					await flushPromises();

					expect(api.updateSettings).toHaveBeenCalledWith(
						expect.objectContaining({ brightness: 77 })
					);
					expect(matrix.hasChanges).toBe(false);
					expect(mockNotifications.success).toHaveBeenCalledWith('matrix updated', 3000);
					resolve();
				});
			});
		});

		cleanup?.();
	});
});
