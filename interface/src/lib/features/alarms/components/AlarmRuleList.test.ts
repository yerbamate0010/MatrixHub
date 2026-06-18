import { fireEvent, render, screen } from '@testing-library/svelte';
import { describe, expect, it, vi } from 'vitest';
import AlarmRuleList from './AlarmRuleList.svelte';
import type { AlarmRule } from '$lib/types/domain/alarms';

vi.mock('$lib/i18n.svelte', () => ({
	i18n: {
		languageTag: 'en'
	}
}));

vi.mock('$lib/paraglide/messages.js', () => ({
	alarms_title: () => 'Alarm Rules',
	alarms_add_btn_with_count: ({ current, max }: { current: number; max: number }) =>
		`Add Rule (${current}/${max})`,
	alarms_info_btn_title: ({ max }: { max: number }) =>
		`How alarms work, available actions and the ${max}-rule limit`,
	alarms_rules_count_status: ({ current, max }: { current: number; max: number }) =>
		`${current}/${max} alarm rules configured`,
	alarms_max_rules_reached: ({ max }: { max: number }) =>
		`Maximum of ${max} alarm rules reached. Remove one to add another.`,
	alarms_no_rules: () => 'No alarm rules configured',
	alarms_no_rules_hint: ({ max }: { max: number }) =>
		`Click "Add Rule" to create your first alarm. You can configure up to ${max} rules.`,
	alarms_severity_info: () => 'Info',
	alarms_severity_warning: () => 'Warning',
	alarms_severity_critical: () => 'Critical',
	aria_edit_rule: () => 'Edit rule',
	aria_delete_rule: () => 'Delete rule',
	source_temperature: () => 'Temperature',
	source_co2: () => 'CO2',
	source_humidity: () => 'Humidity',
	source_wifi_motion: () => 'WiFi Motion',
	source_ble_temperature: () => 'BLE Temperature',
	source_ble_humidity: () => 'BLE Humidity',
	source_ble_battery: () => 'BLE Battery',
	source_ble_rssi: () => 'BLE RSSI'
}));

function createRule(id: string): AlarmRule {
	return {
		id,
		enabled: true,
		name: `Alarm ${id}`,
		source: 'temperature',
		operator: 'above',
		threshold: 30,
		severity: 'warning',
		notify_channels: ['telegram'],
		cooldown_seconds: 600
	};
}

describe('AlarmRuleList', () => {
	it('disables the add button and shows the limit message at 8 rules', async () => {
		const onInfo = vi.fn();
		render(AlarmRuleList, {
			props: {
				rules: Array.from({ length: 8 }, (_, index) => createRule(`${index}`)),
				canManage: true,
				onEdit: vi.fn(),
				onDelete: vi.fn(),
				onToggle: vi.fn(),
				onAdd: vi.fn(),
				onInfo
			}
		});

		const addButton = screen.getByRole('button', { name: 'Add Rule (8/8)' });
		expect((addButton as HTMLButtonElement).disabled).toBe(true);
		expect(
			screen.getByText('Maximum of 8 alarm rules reached. Remove one to add another.')
		).toBeTruthy();

		const infoButton = document.querySelector<HTMLButtonElement>(
			'button[title="How alarms work, available actions and the 8-rule limit"]'
		);
		expect(infoButton).toBeTruthy();
		await fireEvent.click(infoButton!);
		expect(onInfo).toHaveBeenCalledOnce();
	}, 10000);

	it('shows current rule count below the limit', () => {
		render(AlarmRuleList, {
			props: {
				rules: [createRule('1'), createRule('2'), createRule('3')],
				canManage: true,
				onEdit: vi.fn(),
				onDelete: vi.fn(),
				onToggle: vi.fn(),
				onAdd: vi.fn(),
				onInfo: vi.fn()
			}
		});

		expect(
			(screen.getByRole('button', { name: 'Add Rule (3/8)' }) as HTMLButtonElement).disabled
		).toBe(false);
		expect(screen.getByText('3/8 alarm rules configured')).toBeTruthy();
	});

	it('disables mutating controls in read-only mode', () => {
		render(AlarmRuleList, {
			props: {
				rules: [createRule('1')],
				canManage: false,
				onEdit: vi.fn(),
				onDelete: vi.fn(),
				onToggle: vi.fn(),
				onAdd: vi.fn(),
				onInfo: vi.fn()
			}
		});

		expect(
			(screen.getByRole('button', { name: 'Add Rule (1/8)' }) as HTMLButtonElement).disabled
		).toBe(true);
		expect((screen.getByRole('checkbox') as HTMLInputElement).disabled).toBe(true);
		expect((screen.getByRole('button', { name: 'Edit rule' }) as HTMLButtonElement).disabled).toBe(
			true
		);
		expect(
			(screen.getByRole('button', { name: 'Delete rule' }) as HTMLButtonElement).disabled
		).toBe(true);
	});

	it('renders BLE battery and RSSI labels with units', () => {
		render(AlarmRuleList, {
			props: {
				rules: [
					{ ...createRule('battery'), source: 'ble_battery', threshold: 20 },
					{ ...createRule('rssi'), source: 'ble_rssi', threshold: -90 }
				],
				canManage: true,
				onEdit: vi.fn(),
				onDelete: vi.fn(),
				onToggle: vi.fn(),
				onAdd: vi.fn(),
				onInfo: vi.fn()
			}
		});

		expect(screen.getByText(/BLE Battery/)).toBeTruthy();
		expect(screen.getByText(/20%/)).toBeTruthy();
		expect(screen.getByText(/BLE RSSI/)).toBeTruthy();
		expect(screen.getByText(/-90dBm/)).toBeTruthy();
	});
});
