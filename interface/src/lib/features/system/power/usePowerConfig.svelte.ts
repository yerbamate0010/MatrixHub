import type { PowerApiService } from '$lib/services/api/core/PowerApiService';
import type { PowerConfig, PowerStatus } from '$lib/types/system/power';
import { getRequestAbortKind, toUserRequestErrorMessage } from '$lib/utils';
import { notifications } from '$lib/components/toasts/notifications.svelte';
import { i18n } from '$lib/i18n.svelte';
import * as m from '$lib/paraglide/messages.js';

const MS_PER_MIN = 60000;
const INACTIVITY_MIN_MINUTES = 5;
const INACTIVITY_MAX_MINUTES = 1440;
const GRACE_MIN_MINUTES = 1;
const GRACE_MAX_MINUTES = 10;

type PowerSettingsDraft = {
	sleep_enabled: boolean;
	inactivity_timeout_ms: number;
	grace_after_boot_ms: number;
};

type PowerSettingsErrors = {
	inactivity_timeout_ms: boolean;
	grace_after_boot_ms: boolean;
};

function toSettingsDraft(
	status: Pick<PowerStatus, 'sleep_enabled' | 'inactivity_timeout_ms' | 'grace_after_boot_ms'>
): PowerSettingsDraft {
	return {
		sleep_enabled: status.sleep_enabled,
		inactivity_timeout_ms: status.inactivity_timeout_ms,
		grace_after_boot_ms: status.grace_after_boot_ms
	};
}

function createDefaultErrors(): PowerSettingsErrors {
	return {
		inactivity_timeout_ms: false,
		grace_after_boot_ms: false
	};
}

function validatePowerSettings(settings: PowerSettingsDraft, errors: PowerSettingsErrors) {
	const inactivityMinutes = settings.inactivity_timeout_ms / MS_PER_MIN;
	const graceMinutes = settings.grace_after_boot_ms / MS_PER_MIN;

	errors.inactivity_timeout_ms =
		!Number.isFinite(inactivityMinutes) ||
		inactivityMinutes < INACTIVITY_MIN_MINUTES ||
		inactivityMinutes > INACTIVITY_MAX_MINUTES;
	errors.grace_after_boot_ms =
		!Number.isFinite(graceMinutes) ||
		graceMinutes < GRACE_MIN_MINUTES ||
		graceMinutes > GRACE_MAX_MINUTES;

	return errors.inactivity_timeout_ms || errors.grace_after_boot_ms;
}

export function usePowerConfig(
	getApi: () => PowerApiService,
	getStatus: () => PowerStatus | null,
	applyConfig: (config: PowerConfig) => void
) {
	let errorMessage = $state<string | null>(null);
	let saving = $state(false);
	let savedSettings = $state<PowerSettingsDraft | null>(null);
	let settings = $state<PowerSettingsDraft>({
		sleep_enabled: false,
		inactivity_timeout_ms: 0,
		grace_after_boot_ms: 0
	});
	let errors = $state<PowerSettingsErrors>(createDefaultErrors());
	let initialized = $state(false);

	const hasChanges = $derived.by(() => {
		if (!savedSettings) return false;
		return (
			settings.sleep_enabled !== savedSettings.sleep_enabled ||
			settings.inactivity_timeout_ms !== savedSettings.inactivity_timeout_ms ||
			settings.grace_after_boot_ms !== savedSettings.grace_after_boot_ms
		);
	});

	function clearErrors() {
		errors.inactivity_timeout_ms = false;
		errors.grace_after_boot_ms = false;
	}

	function syncFromStatus(status: PowerStatus | null) {
		if (!status) return;
		const nextSettings = toSettingsDraft(status);
		savedSettings = nextSettings;
		if (initialized && hasChanges) return;
		settings = nextSettings;
		clearErrors();
		errorMessage = null;
		initialized = true;
	}

	async function loadSettings() {
		syncFromStatus(getStatus());
	}

	function resetSettings() {
		if (!savedSettings) return;
		settings = savedSettings;
		clearErrors();
		errorMessage = null;
	}

	function toggleSleepEnabled() {
		settings.sleep_enabled = !settings.sleep_enabled;
	}

	async function saveSettingsNow() {
		if (!savedSettings || !hasChanges) return false;

		clearErrors();
		if (validatePowerSettings(settings, errors)) {
			notifications.warning(m.settings_validation_error({ locale: i18n.languageTag }), 3000);
			return false;
		}

		saving = true;
		try {
			const nextConfig: PowerConfig = {
				sleep_enabled: settings.sleep_enabled,
				inactivity_timeout_ms: settings.inactivity_timeout_ms,
				grace_after_boot_ms: settings.grace_after_boot_ms
			};
			const savedConfig = await getApi().updateConfig(nextConfig);
			applyConfig(savedConfig);
			savedSettings = {
				sleep_enabled: savedConfig.sleep_enabled,
				inactivity_timeout_ms: savedConfig.inactivity_timeout_ms,
				grace_after_boot_ms: savedConfig.grace_after_boot_ms
			};
			settings = savedSettings;
			errorMessage = null;
			notifications.success(m.settings_saved({ locale: i18n.languageTag }), 3000);
			return true;
		} catch (nextError) {
			if (getRequestAbortKind(nextError) === 'abort') return false;
			const message = toUserRequestErrorMessage(nextError, {
				fallbackMessage: m.settings_save_error({ locale: i18n.languageTag })
			});
			errorMessage = message;
			notifications.error(m.toast_message({ message }, { locale: i18n.languageTag }), 3000);
			return false;
		} finally {
			saving = false;
		}
	}

	return {
		get settings() {
			return settings;
		},
		get savedSettings() {
			return savedSettings;
		},
		get errors() {
			return errors;
		},
		get errorMessage() {
			return errorMessage;
		},
		get error() {
			return errorMessage;
		},
		get loading() {
			return !initialized && getStatus() === null;
		},
		get saving() {
			return saving;
		},
		get localSleepEnabled() {
			return settings.sleep_enabled;
		},
		set localSleepEnabled(value: boolean) {
			settings.sleep_enabled = value;
		},
		get localInactivityTimeoutMs() {
			return settings.inactivity_timeout_ms;
		},
		set localInactivityTimeoutMs(value: number) {
			settings.inactivity_timeout_ms = value;
			errors.inactivity_timeout_ms = false;
		},
		get localGraceAfterBootMs() {
			return settings.grace_after_boot_ms;
		},
		set localGraceAfterBootMs(value: number) {
			settings.grace_after_boot_ms = value;
			errors.grace_after_boot_ms = false;
		},
		get hasChanges() {
			return hasChanges;
		},
		syncFromStatus,
		loadSettings,
		resetSettings,
		toggleSleepEnabled,
		saveSettingsNow,
		saveSettings: saveSettingsNow
	};
}
