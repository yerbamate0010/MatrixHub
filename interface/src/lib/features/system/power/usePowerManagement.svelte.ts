/**
 * Power management state and API logic (Svelte 5 runes)
 */

import { PowerApiService } from '$lib/services/api/core/PowerApiService';
import { usePowerStatus } from './usePowerStatus.svelte';
import { usePowerConfig } from './usePowerConfig.svelte';

export function usePowerManagement(getApi: () => PowerApiService) {
	const powerStatus = usePowerStatus(getApi);
	const powerConfig = usePowerConfig(getApi, () => powerStatus.status, powerStatus.applyConfig);

	$effect(() => {
		powerConfig.syncFromStatus(powerStatus.status);
	});

	async function restart() {
		await getApi().restart();
	}

	async function factoryReset() {
		await getApi().factoryReset();
	}

	async function requestSleep() {
		await getApi().requestSleep();
	}

	async function requestHygieneSleep() {
		await getApi().requestHygieneSleep();
	}

	return {
		get settings() {
			return powerConfig.settings;
		},
		get savedSettings() {
			return powerConfig.savedSettings;
		},
		get errors() {
			return powerConfig.errors;
		},
		get status() {
			return powerStatus.status;
		},
		get error() {
			return powerConfig.error ?? powerStatus.error;
		},
		get statusError() {
			return powerStatus.error;
		},
		get configError() {
			return powerConfig.error;
		},
		get loading() {
			return powerStatus.loading;
		},
		get saving() {
			return powerConfig.saving;
		},
		get localSleepEnabled() {
			return powerConfig.localSleepEnabled;
		},
		set localSleepEnabled(value: boolean) {
			powerConfig.localSleepEnabled = value;
		},
		get localInactivityTimeoutMs() {
			return powerConfig.localInactivityTimeoutMs;
		},
		set localInactivityTimeoutMs(value: number) {
			powerConfig.localInactivityTimeoutMs = value;
		},
		get localGraceAfterBootMs() {
			return powerConfig.localGraceAfterBootMs;
		},
		set localGraceAfterBootMs(value: number) {
			powerConfig.localGraceAfterBootMs = value;
		},
		get hasChanges() {
			return powerConfig.hasChanges;
		},
		loadSettings: powerConfig.loadSettings,
		resetSettings: powerConfig.resetSettings,
		fetchStatus: powerStatus.fetchStatus,
		toggleSleepEnabled: powerConfig.toggleSleepEnabled,
		saveSettingsNow: powerConfig.saveSettingsNow,
		saveSettings: powerConfig.saveSettings,
		restart,
		factoryReset,
		requestSleep,
		requestHygieneSleep
	};
}
