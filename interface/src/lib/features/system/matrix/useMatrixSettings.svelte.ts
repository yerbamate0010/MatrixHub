import type { MatrixApiService, MatrixSettings } from '$lib/services/api/core/MatrixApiService';
import { createSettingsFeedback } from '$lib/utils/api/settingsFeedback';
import { useSettings } from '$lib/utils/api/useSettings.svelte';
import { i18n } from '$lib/i18n.svelte';
import * as m from '$lib/paraglide/messages.js';
import { DEFAULT_MATRIX_SETTINGS, MATRIX_MENU_BUTTON_LOCKED_ENABLED } from './matrixModel';

type MatrixErrors = Record<string, never>;

type MatrixSettingsDeps = {
	shouldLoad?: () => boolean;
};

export function useMatrixSettings(
	apiFactory: () => MatrixApiService,
	deps: MatrixSettingsDeps = {}
) {
	const feedback = createSettingsFeedback<MatrixSettings>({
		loadErrorFallback: () => m.matrix_err_load({ locale: i18n.languageTag }),
		saveErrorFallback: () => m.matrix_err_save({ locale: i18n.languageTag }),
		success: () => m.toast_settings_updated({ locale: i18n.languageTag })
	});

	const hook = useSettings<MatrixSettings, MatrixErrors>(
		DEFAULT_MATRIX_SETTINGS,
		{},
		{
			load: async () => {
				return await apiFactory().getSettings();
			},
			save: async (settings) => {
				return await apiFactory().updateSettings({
					...settings,
					menu_enabled: MATRIX_MENU_BUTTON_LOCKED_ENABLED
				});
			},
			feedback
		}
	);

	let autoLoadArmed = true;

	$effect(() => {
		if (deps.shouldLoad === undefined) return;
		if (!deps.shouldLoad()) {
			autoLoadArmed = true;
			hook.loading = false;
			return;
		}
		if (!autoLoadArmed) return;
		autoLoadArmed = false;
		void hook.loadSettings();
	});

	return {
		get error() {
			return hook.errorMessage ?? undefined;
		},
		get loading() {
			return hook.loading;
		},
		set loading(value: boolean) {
			hook.loading = value;
		},
		get saving() {
			return hook.saving;
		},
		get hasChanges() {
			return hook.hasChanges;
		},
		get settings() {
			return hook.settings;
		},
		set settings(nextSettings: MatrixSettings) {
			hook.setSettings(nextSettings);
		},
		loadSettings: hook.loadSettings,
		saveSettingsNow: hook.saveSettingsNow,
		saveSettingsSilentlyNow: hook.saveSettingsSilentlyNow,
		saveSettings: hook.saveSettings,
		resetSettings: hook.resetSettings,
		updateSetting: hook.updateSetting
	};
}
