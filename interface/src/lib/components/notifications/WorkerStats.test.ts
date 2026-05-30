import { render, screen, waitFor } from '@testing-library/svelte';
import { describe, expect, it, vi, beforeEach } from 'vitest';
import WorkerStats from './WorkerStats.svelte';

const { subscribeMock, destroyMock, emitEvent, setOnEventHandler } = vi.hoisted(() => {
	let onEvent: ((event: unknown) => void) | undefined;

	return {
		subscribeMock: vi.fn(),
		destroyMock: vi.fn(),
		setOnEventHandler: (callback?: (event: unknown) => void) => {
			onEvent = callback;
		},
		emitEvent: (event: unknown) => {
			onEvent?.(event);
		}
	};
});

vi.mock('$lib/stores/system/channelSubscription.svelte', () => ({
	createSystemChannelSubscription: vi.fn((options: { onEvent?: (event: unknown) => void }) => {
		setOnEventHandler(options.onEvent);
		return {
			subscribe: subscribeMock,
			destroy: destroyMock
		};
	})
}));

vi.mock('$lib/paraglide/messages.js', () => ({
	notif_type_webhook: () => 'Webhook',
	notif_type_pushover: () => 'Pushover',
	notif_type_udp: () => 'UDP',
	notif_type_heartbeat: () => 'Heartbeat',
	telegram_title: () => 'Telegram',
	notif_worker_title: ({ type }: { type: string }) => `Worker: ${type}`,
	notif_worker_last_activity: () => 'Last activity',
	notif_worker_messages: () => 'Messages',
	notif_worker_http_code: () => 'HTTP code',
	telegram_worker_status: () => 'Status',
	telegram_worker_running: () => 'Running',
	telegram_worker_starting: () => 'Starting',
	telegram_worker_disabled: () => 'Disabled',
	telegram_worker_messages: () => 'Telegram messages',
	telegram_worker_messages_stats: ({ in: inbound, out }: { in: number; out: number }) =>
		`${inbound}/${out}`,
	telegram_worker_commands: () => 'Commands',
	notif_worker_last_sync: () => 'Last sync',
	notif_worker_packets: () => 'Packets',
	notif_worker_slot: ({ n }: { n: number }) => `Slot ${n}`,
	notif_worker_ok: () => 'OK',
	notif_worker_err: () => 'ERR'
}));

vi.mock('$lib/i18n.svelte', () => ({
	i18n: {
		languageTag: 'en'
	}
}));

describe('WorkerStats', () => {
	beforeEach(() => {
		vi.clearAllMocks();
	});

	it('subscribes to notif_stats and renders webhook stats from incoming events', async () => {
		const view = render(WorkerStats, { type: 'webhook' });

		expect(subscribeMock).toHaveBeenCalledWith({ hydrateSnapshot: false });

		emitEvent({
			type: 'notif_stats',
			data: {
				webhook: { sent: 5, failed: 1, lastMs: 2000, httpCode: 204 },
				pushover: { sent: 0, failed: 0, lastMs: 0, httpCode: 0 },
				udp: { sent: 0, failed: 0, lastMs: 0 },
				heartbeat: [],
				telegram: {
					enabled: false,
					running: false,
					lastActivityMs: 0,
					messagesProcessed: 0,
					messagesSent: 0,
					commandsExecuted: 0,
					lastHttpCode: 0
				},
				uptimeMs: 5000
			}
		});

		await waitFor(() => {
			expect(screen.getByText('Worker: Webhook')).toBeTruthy();
			expect(screen.getByText('3s')).toBeTruthy();
			expect(screen.getByText('5')).toBeTruthy();
			expect(screen.getByText('1')).toBeTruthy();
			expect(screen.getByText('204')).toBeTruthy();
		});

		view.unmount();
		expect(destroyMock).toHaveBeenCalledTimes(1);
	});
});
