import type { AlarmRule } from '$lib/types/domain/alarms';
import { i18n } from '$lib/i18n.svelte';
import * as m from '$lib/paraglide/messages.js';
import { notifications } from '$lib/components/toasts/notifications.svelte';
import { useAlarmRulesSource } from '$lib/features/alarms/useAlarmRulesSource.svelte';

export function useAlarmsWidgetManagement() {
	const source = useAlarmRulesSource({ loadFromApiOnStart: false });

	const hasRules = $derived(source.rules.length > 0);
	const hasActiveAlarms = $derived(source.rules.some((rule) => rule.triggered));
	const canManage = $derived(source.canManage);

	async function toggleRule(rule: AlarmRule, event: Event) {
		event.stopPropagation();
		if (!canManage) return;

		const success = await source.toggleRule(rule);
		if (!success && source.errorMessage) {
			notifications.error(
				m.toast_message({ message: source.errorMessage }, { locale: i18n.languageTag }),
				4000
			);
		}
	}

	function getSourceLabel(sourceType: AlarmRule['source']) {
		switch (sourceType) {
			case 'co2':
				return m.source_co2({ locale: i18n.languageTag });
			case 'temperature':
				return m.source_temperature({ locale: i18n.languageTag });
			case 'humidity':
				return m.source_humidity({ locale: i18n.languageTag });
			case 'wifi_motion':
				return m.source_wifi_motion({ locale: i18n.languageTag });
			case 'ble_temperature':
				return m.source_ble_temperature({ locale: i18n.languageTag });
			case 'ble_humidity':
				return m.source_ble_humidity({ locale: i18n.languageTag });
			case 'ble_battery':
				return m.source_ble_battery({ locale: i18n.languageTag });
			case 'ble_rssi':
				return m.source_ble_rssi({ locale: i18n.languageTag });
		}
	}

	return {
		get rules() {
			return source.rules;
		},
		get loading() {
			return source.loading;
		},
		get errorMessage() {
			return source.errorMessage;
		},
		get hasRules() {
			return hasRules;
		},
		get hasActiveAlarms() {
			return hasActiveAlarms;
		},
		get canManage() {
			return canManage;
		},
		toggleRule,
		getSourceLabel
	};
}
