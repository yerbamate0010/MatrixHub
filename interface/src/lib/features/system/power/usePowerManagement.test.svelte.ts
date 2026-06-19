import { beforeEach, describe, expect, it, vi } from 'vitest';
import { usePowerManagement } from './usePowerManagement.svelte';
import type { PowerApiService } from '$lib/services/api/core/PowerApiService';

const { mockNotifications } = vi.hoisted(() => ({
	mockNotifications: {
		success: vi.fn(),
		error: vi.fn(),
		warning: vi.fn()
	}
}));

vi.mock('$lib/utils/api/usePolling.svelte', () => ({
	usePolling: vi.fn(() => ({
		start: vi.fn(),
		stop: vi.fn()
	}))
}));

vi.mock('$lib/components/toasts/notifications.svelte', () => ({
	notifications: mockNotifications
}));

vi.mock('$lib/utils', () => ({
	getRequestAbortKind: vi.fn(() => null),
	toUserRequestErrorMessage: vi.fn((error: unknown, options?: { fallbackMessage?: string }) => {
		if (error instanceof Error && error.message) return error.message;
		return options?.fallbackMessage ?? 'unknown error';
	})
}));

vi.mock('$lib/i18n.svelte', () => ({
	i18n: {
		languageTag: 'en'
	}
}));

vi.mock('$lib/paraglide/messages.js', () => ({
	settings_save_error: () => 'Failed to save settings.',
	settings_saved: () => 'Settings saved.',
	toast_message: ({ message }: { message: string }) => message,
	duration_sleeping_soon: () => 'soon',
	duration_seconds: ({ seconds }: { seconds: string }) => `${seconds}s`,
	duration_minutes: ({ minutes }: { minutes: string }) => `${minutes}m`,
	duration_hours: ({ hours }: { hours: string }) => `${hours}h`
}));

describe('usePowerManagement', () => {
	const api = {
		getStatus: vi.fn(),
		updateConfig: vi.fn(),
		restart: vi.fn(),
		factoryReset: vi.fn(),
		requestSleep: vi.fn(),
		requestHygieneSleep: vi.fn()
	};

	beforeEach(() => {
		vi.clearAllMocks();
		api.getStatus.mockResolvedValue({
			wake_reason: 'timer',
			wake_cause_raw: 1,
			wake_gpio_mask: '0',
			wake_ext1_mask: '0',
			sleep_requested: false,
			sleep_eta_ms: 0,
			inactivity_timeout_ms: 600000,
			grace_after_boot_ms: 120000,
			wake_interval_ms: 300000,
			last_activity_ms: 1000,
			uptime_ms: 10000,
			thermal_state: 'normal',
			thermal_temp_c: 44.5,
			thermal_cpu_mhz: 240,
			thermal_throttled: false,
			thermal_soft_c: 60,
			thermal_hard_c: 68,
			thermal_critical_c: 80,
			sleep_enabled: true
		});
		api.updateConfig.mockResolvedValue({
			sleep_enabled: false,
			inactivity_timeout_ms: 900000,
			grace_after_boot_ms: 120000
		});
	});

	it('syncs local draft from fetched power status', async () => {
		let cleanup: (() => void) | undefined;

		await new Promise<void>((resolve) => {
			cleanup = $effect.root(() => {
				const power = usePowerManagement(() => api as unknown as PowerApiService);

				void power.fetchStatus().then(() => {
					setTimeout(() => {
						expect(power.status?.sleep_enabled).toBe(true);
						expect(power.localSleepEnabled).toBe(true);
						expect(power.localInactivityTimeoutMs).toBe(600000);
						expect(power.status?.thermal_state).toBe('normal');
						expect(power.status?.thermal_cpu_mhz).toBe(240);
						expect(power.hasChanges).toBe(false);
						resolve();
					}, 0);
				});
			});
		});

		cleanup?.();
	});

	it('saves config through config layer and keeps status in sync', async () => {
		let cleanup: (() => void) | undefined;

		await new Promise<void>((resolve) => {
			cleanup = $effect.root(() => {
				const power = usePowerManagement(() => api as unknown as PowerApiService);

				void power.fetchStatus().then(async () => {
					power.localSleepEnabled = false;
					power.localInactivityTimeoutMs = 900000;

					await power.saveSettings();

					expect(api.updateConfig).toHaveBeenCalledWith({
						sleep_enabled: false,
						inactivity_timeout_ms: 900000,
						grace_after_boot_ms: 120000
					});
					expect(power.status?.sleep_enabled).toBe(false);
					expect(power.status?.inactivity_timeout_ms).toBe(900000);
					expect(power.configError).toBeNull();
					expect(power.hasChanges).toBe(false);
					resolve();
				});
			});
		});

		cleanup?.();
	});
});
