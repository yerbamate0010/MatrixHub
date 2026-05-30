import { systemStatus } from '../systemStatus.svelte';

interface SystemTransportStateLike {
	readonly isConnected: boolean;
}

interface SystemTransportControllerLike {
	ensureConnected(): void;
	connect(): void;
}

interface SystemTransportStateDeps {
	runtime?: SystemTransportStateLike;
}

interface SystemTransportControllerDeps {
	runtime?: SystemTransportControllerLike;
}

export function useSystemTransportState(deps: SystemTransportStateDeps = {}) {
	const runtime = deps.runtime ?? systemStatus;

	return {
		get isConnected() {
			return runtime.isConnected;
		}
	};
}

export function useSystemTransportController(deps: SystemTransportControllerDeps = {}) {
	const runtime = deps.runtime ?? systemStatus;

	return {
		ensureConnected() {
			runtime.ensureConnected();
		},
		connect() {
			runtime.connect();
		}
	};
}
