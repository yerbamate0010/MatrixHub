import { beforeEach, describe, expect, it, vi } from 'vitest';

const { mockPage, mockUser, refs, modalStack, mockNotifications, mockCreateService } = vi.hoisted(
	() => ({
		mockPage: {
			data: {
				features: {
					ntp: true,
					sleep: true
				},
				title: '',
				titleOverride: false
			},
			url: new URL('http://localhost/charts')
		},
		mockUser: {
			admin: true,
			bearer_token: 'token'
		},
		refs: {
			systemStatus: null as unknown
		},
		modalStack: {
			open: vi.fn(),
			close: vi.fn(),
			closeById: vi.fn()
		},
		mockNotifications: {
			error: vi.fn()
		},
		mockCreateService: vi.fn()
	})
);

class MockSystemStatusStore {
	data = $state({
		timestamp: 0,
		lastUpdate: 0,
		wifiStatus: 0,
		rssi: -60,
		isConnected: true,
		isStaConnected: true,
		isApMode: false,
		coreTemp: 44
	});

	setData(next: Partial<typeof this.data>) {
		Object.assign(this.data, next);
	}
}

const mockSystemStatusStore = new MockSystemStatusStore();
refs.systemStatus = mockSystemStatusStore;

vi.mock('svelte', async (importOriginal) => {
	const actual = await importOriginal<typeof import('svelte')>();
	return {
		...actual,
		onMount: (fn: () => void) => fn(),
		onDestroy: vi.fn()
	};
});

vi.mock('$app/state', () => ({
	page: mockPage
}));

vi.mock('$lib/stores/user', () => ({
	user: mockUser
}));

vi.mock('$lib/stores/systemStatus.svelte', () => ({
	systemStatus: refs.systemStatus
}));

vi.mock('$lib/i18n.svelte', () => ({
	i18n: {
		languageTag: 'en'
	}
}));

vi.mock('$lib/utils/api/useApiClient.svelte', () => ({
	useApiClient: () => ({
		createService: mockCreateService
	})
}));

vi.mock('$lib/components', () => ({
	ConfirmDialog: {}
}));

vi.mock('$lib/components/toasts/notifications.svelte', () => ({
	notifications: mockNotifications
}));

vi.mock('$lib/utils/ui/dialogs', () => ({
	confirm: vi.fn((options: { modalService?: typeof modalStack } & Record<string, unknown>) => {
		const { modalService = modalStack, component: _component, ...props } = options;
		return modalService.open({}, props);
	})
}));

vi.mock('$lib/utils/ui/modal', () => ({
	defaultModalStack: modalStack,
	openModal: vi.fn(),
	closeModalById: vi.fn()
}));

vi.mock('$lib/utils', () => ({
	toUserRequestErrorMessage: vi.fn(() => 'sleep failed')
}));

vi.mock('./featureRegistry', () => ({
	resolveFeatureTitle: vi.fn(() => 'Charts')
}));

vi.mock('$lib/paraglide/messages.js', () => ({
	app_title: () => 'MatrixHub',
	sleep_confirm_title: () => 'Sleep?',
	sleep_confirm_msg: () => 'Enter sleep?',
	sleep_abort_btn: () => 'Cancel',
	sleep_enter_btn: () => 'Sleep',
	sleep_request_failed: () => 'Sleep failed.',
	error_prefix: ({ error }: { error: string }) => `Error: ${error}`
}));

describe('useStatusbarManagement', () => {
	beforeEach(() => {
		vi.clearAllMocks();
		mockPage.data.title = '';
		mockPage.data.titleOverride = false;
		mockPage.url = new URL('http://localhost/charts');
		mockSystemStatusStore.setData({
			isConnected: true,
			isStaConnected: true
		});
		mockCreateService.mockReturnValue({ requestSleep: vi.fn().mockResolvedValue(undefined) });
	});

	it('shows error toast when sleep request fails', async () => {
		const requestSleep = vi.fn().mockRejectedValue(new Error('offline'));
		mockCreateService.mockReturnValue({ requestSleep });
		const { useStatusbarManagement } = await import('./useStatusbarManagement.svelte');
		let cleanup: (() => void) | undefined;

		cleanup = $effect.root(() => {
			const statusbar = useStatusbarManagement({
				modalStack,
				now: () => 0
			});

			statusbar.confirmSleep();
			const openCall = modalStack.open.mock.calls.at(-1);
			expect(openCall).toBeTruthy();
			const props = openCall?.[1] as { onConfirm: () => void };

			void props.onConfirm();
		});

		await vi.waitFor(() => {
			expect(requestSleep).toHaveBeenCalledOnce();
			expect(mockNotifications.error).toHaveBeenCalledWith('Error: sleep failed', 4000);
		});

		cleanup?.();
	});
});
