import type { ApStatus } from '$lib/types/connectivity/ap';
import { i18n } from '$lib/i18n.svelte';
import * as m from '$lib/paraglide/messages.js';
import { createSystemStatusBackedStatus } from '../systemStatusBackedStatus.svelte';
import type { SystemStatusSnapshot } from '$lib/types/system/systemStatusSnapshot';

const ERROR_COOLDOWN_MS = 30000;

const DEFAULT_STATUS: ApStatus = {
	// Match the legacy /rest/apStatus contract used by ApStatusCard:
	// 0 = active, 1 = inactive. Default to inactive until a snapshot arrives.
	status: 1,
	ip_address: '0.0.0.0',
	mac_address: '00:00:00:00:00:00',
	station_num: 0
};

function mapApStatusSnapshot(snapshot: SystemStatusSnapshot): ApStatus | null {
	const ap = snapshot.diagnostics?.ap;
	if (!snapshot.diagnostics) return null;

	return {
		// Keep the websocket-backed value aligned with the old REST enum so the
		// AP card and any legacy consumers render the same active/inactive state.
		status: ap?.active ? 0 : 1,
		ip_address: ap?.ip ?? '0.0.0.0',
		mac_address: ap?.mac ?? '00:00:00:00:00:00',
		station_num: ap?.stationNum ?? 0
	};
}

export function useApStatus() {
	const statusSync = createSystemStatusBackedStatus({
		defaultStatus: DEFAULT_STATUS,
		mapSnapshot: mapApStatusSnapshot,
		errorCooldownMs: ERROR_COOLDOWN_MS,
		fallbackMessage: () => m.toast_ap_status_load_failed({ locale: i18n.languageTag }),
		loggerLabel: 'AP status'
	});

	return {
		get status() {
			return statusSync.status;
		},
		get error() {
			return statusSync.error;
		},
		start: statusSync.start,
		refresh: statusSync.refresh
	};
}
