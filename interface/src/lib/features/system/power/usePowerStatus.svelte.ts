import type { PowerApiService } from '$lib/services/api/core/PowerApiService';
import type { PowerConfig, PowerStatus } from '$lib/types/system/power';
import { getRequestAbortKind, toUserRequestErrorMessage } from '$lib/utils';
import { usePolling } from '$lib/utils/api/usePolling.svelte';

const STATUS_POLL_INTERVAL_MS = 5000;

export function usePowerStatus(getApi: () => PowerApiService) {
	let status = $state<PowerStatus | null>(null);
	let error = $state<string | null>(null);
	let loading = $state(true);

	async function fetchStatus() {
		try {
			status = await getApi().getStatus();
			error = null;
		} catch (nextError) {
			if (getRequestAbortKind(nextError) === 'abort') return;
			error = toUserRequestErrorMessage(nextError);
		} finally {
			loading = false;
		}
	}

	usePolling(fetchStatus, {
		pauseWhenHidden: true,
		intervalMs: STATUS_POLL_INTERVAL_MS,
		jitter: true
	});

	function applyConfig(snapshot: PowerConfig) {
		if (!status) return;
		status.sleep_enabled = snapshot.sleep_enabled;
		status.inactivity_timeout_ms = snapshot.inactivity_timeout_ms;
		status.grace_after_boot_ms = snapshot.grace_after_boot_ms;
	}

	return {
		get status() {
			return status;
		},
		get error() {
			return error;
		},
		get loading() {
			return loading;
		},
		fetchStatus,
		applyConfig
	};
}
