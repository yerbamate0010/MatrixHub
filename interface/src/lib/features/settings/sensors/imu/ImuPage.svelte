<script lang="ts">
	import Activity from '~icons/tabler/activity';
	import Bell from '~icons/tabler/bell';
	import ExternalLink from '~icons/tabler/external-link';
	import Gauge from '~icons/tabler/gauge';
	import GridDots from '~icons/tabler/grid-dots';
	import Mouse from '~icons/tabler/mouse';
	import Rotate from '~icons/tabler/rotate-clockwise';
	import Target from '~icons/tabler/target';
	import Save from '~icons/tabler/device-floppy';
	import Refresh from '~icons/tabler/refresh';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';
	import { AdminAccessGate, GridLayout } from '$lib/components/layout';
	import BaseCard from '$lib/components/layout/BaseCard.svelte';
	import StatusRow from '$lib/components/layout/StatusRow.svelte';
	import ContentBox from '$lib/components/layout/ContentBox.svelte';
	import { Spinner } from '$lib/components';
	import { FormButton, FormRange, FormToggle } from '$lib/components/shared/forms';
	import { useSessionAccess } from '$lib/features/auth/useSessionAccess.svelte';
	import { useImuConsumerSettings } from './useImuConsumerSettings.svelte';
	import { useImuSettings } from './useImuSettings.svelte';

	const session = useSessionAccess();
	const canManage = $derived(session.canManage);
	const imu = useImuSettings({ shouldLoad: () => canManage });
	const consumerSettings = useImuConsumerSettings({ shouldLoad: () => canManage });

	const status = $derived(imu.status);
	const metrics = $derived(status?.metrics ?? null);
	const alarm = $derived(status?.alarm ?? null);
	const consumerRows = $derived.by(() => {
		const consumers = status?.consumers;
		return [
			{
				key: 'ui_monitor',
				label: m.imu_consumer_ui_monitor({ locale: i18n.languageTag }),
				icon: Activity,
				configured: imu.settings.ui_monitor_enabled,
				data: consumers?.ui_monitor
			},
			{
				key: 'alarm',
				label: m.imu_consumer_alarm({ locale: i18n.languageTag }),
				icon: Bell,
				configured: imu.settings.alarm_monitor_enabled,
				data: consumers?.alarm
			},
			{
				key: 'auto_rotate',
				label: m.imu_consumer_matrix({ locale: i18n.languageTag }),
				icon: GridDots,
				configured: consumerSettings.matrixAutoRotate,
				data: consumers?.auto_rotate,
				href: '/system/matrix'
			},
			{
				key: 'airmouse_movement',
				label: m.imu_consumer_airmouse_movement({ locale: i18n.languageTag }),
				icon: Mouse,
				configured: consumerSettings.airMouseMovementEnabled,
				data: consumers?.airmouse_movement,
				href: '/usb-features/airmouse'
			},
			{
				key: 'airmouse_click',
				label: m.imu_consumer_airmouse_click({ locale: i18n.languageTag }),
				icon: Mouse,
				configured: consumerSettings.airMouseClickEnabled,
				data: consumers?.airmouse_click,
				href: '/usb-features/airmouse'
			}
		];
	});

	function yesNo(value?: boolean) {
		return value ? 'OK' : '--';
	}

	function freshLabel(value?: boolean) {
		return value ? 'fresh' : 'stale';
	}

	function enabledLabel(value: boolean | null | undefined) {
		if (value === null || value === undefined) return m.imu_state_unknown({ locale: i18n.languageTag });
		return value
			? m.imu_state_enabled({ locale: i18n.languageTag })
			: m.imu_state_disabled({ locale: i18n.languageTag });
	}

	function runtimeLabel(desired?: boolean, running?: boolean) {
		const desiredLabel = desired
			? m.imu_state_desired({ locale: i18n.languageTag })
			: m.imu_state_not_desired({ locale: i18n.languageTag });
		const runningLabel = running
			? m.imu_state_running({ locale: i18n.languageTag })
			: m.imu_state_idle({ locale: i18n.languageTag });
		return `${desiredLabel} · ${runningLabel}`;
	}

	function sampleAgeLabel(value: number | null | undefined) {
		if (value === null || value === undefined) return '--';
		return `${value} ms`;
	}

	function numberLabel(value: number | null | undefined, digits = 2, suffix = '') {
		if (value === null || value === undefined || Number.isNaN(value)) return '--';
		return `${value.toFixed(digits)}${suffix}`;
	}

	function vectorLabel(vector: { x: number; y: number; z: number } | undefined) {
		if (!vector) return '--';
		return `${vector.x.toFixed(3)}, ${vector.y.toFixed(3)}, ${vector.z.toFixed(3)}`;
	}

	function alarmReasonLabel(reason: string | undefined) {
		switch (reason) {
			case 'tilt':
				return m.imu_alarm_reason_tilt({ locale: i18n.languageTag });
			case 'shock':
				return m.imu_alarm_reason_shock({ locale: i18n.languageTag });
			case 'stale':
				return m.imu_alarm_reason_stale({ locale: i18n.languageTag });
			case 'no_baseline':
				return m.imu_alarm_reason_no_baseline({ locale: i18n.languageTag });
			case 'unavailable':
				return m.imu_alarm_reason_unavailable({ locale: i18n.languageTag });
			case 'none':
				return m.imu_alarm_reason_none({ locale: i18n.languageTag });
			default:
				return '--';
		}
	}
</script>

<AdminAccessGate allow={canManage}>
	<GridLayout cols={2}>
		<BaseCard title={m.imu_status({ locale: i18n.languageTag })} icon={Activity} class="h-full">
			{#if imu.loading && !status}
				<div class="flex items-center justify-center py-8">
					<Spinner />
				</div>
			{:else}
				<div class="space-y-2">
					<StatusRow
						icon={Gauge}
						label={m.imu_running({ locale: i18n.languageTag })}
						value={yesNo(status?.running)}
						subValue={status?.running_consumers ?? 'none'}
					/>
					<StatusRow
						icon={Activity}
						label={m.imu_initialized({ locale: i18n.languageTag })}
						value={yesNo(status?.initialized)}
						subValue={status?.desired_consumers ?? 'none'}
					/>
					<StatusRow
						icon={Refresh}
						label={m.imu_sample({ locale: i18n.languageTag })}
						value={freshLabel(status?.sample_fresh)}
						subValue={sampleAgeLabel(status?.sample_age_ms)}
					/>
					<StatusRow
						icon={Target}
						label={m.imu_last_error({ locale: i18n.languageTag })}
						value={status?.last_start_error ?? '--'}
						subValue={status?.retry_pending ? `retry @ ${status.next_retry_ms} ms` : undefined}
					/>

					<ContentBox>
						<div class="grid grid-cols-2 gap-3 text-sm md:grid-cols-3">
							<div>
								<div class="text-xs opacity-60">accel</div>
								<div class="font-mono">{numberLabel(metrics?.accel_magnitude_g, 3, ' g')}</div>
							</div>
							<div>
								<div class="text-xs opacity-60">delta</div>
								<div class="font-mono">{numberLabel(metrics?.accel_delta_g, 3, ' g')}</div>
							</div>
							<div>
								<div class="text-xs opacity-60">gyro</div>
								<div class="font-mono">{numberLabel(metrics?.gyro_magnitude_dps, 1, ' dps')}</div>
							</div>
							<div>
								<div class="text-xs opacity-60">tilt</div>
								<div class="font-mono">{numberLabel(metrics?.tilt_deg, 1, '°')}</div>
							</div>
							<div class="col-span-2">
								<div class="text-xs opacity-60">baseline</div>
								<div class="font-mono">{vectorLabel(metrics?.orientation_baseline)}</div>
							</div>
						</div>
					</ContentBox>
					<ContentBox>
						<div class="grid grid-cols-2 gap-3 text-sm md:grid-cols-3">
							<div>
								<div class="text-xs opacity-60">{m.imu_alarm_state({ locale: i18n.languageTag })}</div>
								<div class={alarm?.triggered ? 'font-semibold text-error' : 'font-semibold text-success'}>
									{alarm?.triggered
										? m.imu_alarm_triggered({ locale: i18n.languageTag })
										: m.imu_alarm_clear({ locale: i18n.languageTag })}
								</div>
							</div>
							<div>
								<div class="text-xs opacity-60">{m.imu_alarm_reason({ locale: i18n.languageTag })}</div>
								<div class="font-mono">{alarmReasonLabel(alarm?.reason)}</div>
							</div>
							<div>
								<div class="text-xs opacity-60">{m.imu_current_tilt({ locale: i18n.languageTag })}</div>
								<div class="font-mono">{numberLabel(alarm?.tilt_deg, 1, '°')}</div>
							</div>
							<div>
								<div class="text-xs opacity-60">{m.imu_current_accel_delta({ locale: i18n.languageTag })}</div>
								<div class="font-mono">{numberLabel(alarm?.accel_delta_g, 3, ' g')}</div>
							</div>
							<div>
								<div class="text-xs opacity-60">{m.imu_alarm_trigger_hold({ locale: i18n.languageTag })}</div>
								<div class="font-mono">{alarm?.trigger_hold_elapsed_ms ?? 0} ms</div>
							</div>
							<div>
								<div class="text-xs opacity-60">{m.imu_alarm_clear_hold({ locale: i18n.languageTag })}</div>
								<div class="font-mono">{alarm?.clear_hold_elapsed_ms ?? 0} ms</div>
							</div>
						</div>
					</ContentBox>
				</div>
			{/if}
		</BaseCard>

		<BaseCard title={m.imu_consumers({ locale: i18n.languageTag })} icon={Rotate} class="h-full">
			<div class="space-y-2">
				{#if consumerSettings.errorMessage}
					<ContentBox>
						<div class="text-warning text-sm">{consumerSettings.errorMessage}</div>
					</ContentBox>
				{/if}
				{#each consumerRows as row (row.key)}
					<ContentBox>
						<div class="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
							<StatusRow
								icon={row.icon}
								label={row.label}
								value={enabledLabel(row.configured)}
								subValue={runtimeLabel(row.data?.desired, row.data?.running)}
							/>
							<div class="flex items-center justify-end gap-2">
								{#if row.key === 'auto_rotate'}
									<input
										type="checkbox"
										class="toggle toggle-primary toggle-sm"
										aria-label={m.imu_consumer_matrix({ locale: i18n.languageTag })}
										checked={consumerSettings.matrixAutoRotate === true}
										disabled={!canManage ||
											consumerSettings.matrixAutoRotate === null ||
											consumerSettings.matrixSaving}
										onchange={async (event) => {
											await consumerSettings.setMatrixAutoRotate(
												(event.target as HTMLInputElement).checked
											);
											await imu.refreshStatus();
										}}
									/>
								{/if}
								{#if row.href}
									<a class="btn btn-outline btn-xs" href={row.href} aria-label={row.label}>
										<ExternalLink class="h-3.5 w-3.5" />
										{m.imu_open_config({ locale: i18n.languageTag })}
									</a>
								{/if}
							</div>
						</div>
					</ContentBox>
				{/each}
			</div>
		</BaseCard>

		<BaseCard title={m.imu_settings({ locale: i18n.languageTag })} icon={Gauge} class="h-full">
			{#if imu.loading}
				<div class="flex items-center justify-center py-8">
					<Spinner />
				</div>
			{:else}
				<form
					class="space-y-2"
					onsubmit={(event) => {
						event.preventDefault();
						imu.saveSettings();
					}}
				>
					<FormToggle
						label={m.imu_live_monitor({ locale: i18n.languageTag })}
						description={m.imu_live_monitor_desc({ locale: i18n.languageTag })}
						checked={imu.settings.ui_monitor_enabled}
						onchange={(event) =>
							imu.updateSetting('ui_monitor_enabled', (event.target as HTMLInputElement).checked)}
					/>
					<FormToggle
						label={m.imu_alarm_monitor({ locale: i18n.languageTag })}
						description={m.imu_alarm_monitor_desc({ locale: i18n.languageTag })}
						checked={imu.settings.alarm_monitor_enabled}
						onchange={(event) =>
							imu.updateSetting('alarm_monitor_enabled', (event.target as HTMLInputElement).checked)}
					/>
					<ContentBox>
						<FormRange
							id="imu_tilt_threshold"
							label={m.imu_tilt_threshold({ locale: i18n.languageTag })}
							bind:value={imu.settings.tilt_threshold_deg}
							min={1}
							max={90}
							step={1}
							suffix="°"
						/>
					</ContentBox>
					<ContentBox>
						<FormRange
							id="imu_tilt_hysteresis"
							label={m.imu_tilt_hysteresis({ locale: i18n.languageTag })}
							bind:value={imu.settings.tilt_hysteresis_deg}
							min={0}
							max={30}
							step={1}
							suffix="°"
						/>
					</ContentBox>
					<ContentBox>
						<FormRange
							id="imu_accel_delta"
							label={m.imu_accel_delta_threshold({ locale: i18n.languageTag })}
							bind:value={imu.settings.accel_delta_threshold_g}
							min={0.01}
							max={2}
							step={0.01}
							suffix="g"
						/>
					</ContentBox>
					<ContentBox>
						<FormRange
							id="imu_tilt_hold"
							label={m.imu_tilt_hold({ locale: i18n.languageTag })}
							bind:value={imu.settings.tilt_hold_ms}
							min={100}
							max={10000}
							step={50}
							suffix="ms"
						/>
					</ContentBox>
					<ContentBox>
						<FormRange
							id="imu_tilt_clear_hold"
							label={m.imu_tilt_clear_hold({ locale: i18n.languageTag })}
							bind:value={imu.settings.tilt_clear_hold_ms}
							min={100}
							max={15000}
							step={50}
							suffix="ms"
						/>
					</ContentBox>
					<div class="flex justify-end gap-2 pt-2">
						<FormButton
							variant="ghost"
							icon={Refresh}
							ariaLabel={m.action_discard({ locale: i18n.languageTag })}
							title={m.action_discard({ locale: i18n.languageTag })}
							disabled={!imu.hasChanges || imu.saving}
							onclick={imu.resetSettings}
						/>
						<FormButton
							type="submit"
							icon={Save}
							label={m.action_save({ locale: i18n.languageTag })}
							loading={imu.saving}
							disabled={!imu.hasChanges || imu.saving}
						/>
					</div>
				</form>
			{/if}
		</BaseCard>

		<BaseCard title={m.imu_calibration({ locale: i18n.languageTag })} icon={Target} class="h-full">
			<div class="space-y-2">
				<StatusRow
					icon={Target}
					label="baseline"
					value={imu.settings.orientation_baseline_valid ? 'valid' : 'missing'}
					subValue={vectorLabel(imu.settings.orientation_baseline)}
				/>
				<StatusRow
					icon={Refresh}
					label="revision"
					value={imu.settings.calibration_revision}
					subValue={`${imu.settings.baseline_calibrated_at} ms`}
				/>
				{#if imu.calibrationResult}
					<ContentBox>
						<div class={imu.calibrationResult.ok ? 'text-success' : 'text-warning'}>
							{imu.calibrationResult.ok
								? m.imu_calibration_success({ locale: i18n.languageTag })
								: m.imu_calibration_failed({ locale: i18n.languageTag })}
						</div>
						<div class="text-xs opacity-70">
							{imu.calibrationResult.status} · {imu.calibrationResult.sample_count} samples ·
							{numberLabel(imu.calibrationResult.accel_magnitude_variance, 4)}
						</div>
					</ContentBox>
				{/if}
				<div class="flex justify-end gap-2 pt-2">
					<FormButton
						variant="ghost"
						icon={Refresh}
						label={m.imu_reset_orientation({ locale: i18n.languageTag })}
						loading={imu.resettingBaseline}
						disabled={imu.calibrating || imu.resettingBaseline}
						onclick={imu.resetOrientationBaseline}
					/>
					<FormButton
						icon={Target}
						label={m.imu_calibrate_orientation({ locale: i18n.languageTag })}
						loading={imu.calibrating}
						disabled={imu.calibrating || imu.resettingBaseline}
						onclick={imu.calibrateOrientation}
					/>
				</div>
			</div>
		</BaseCard>
	</GridLayout>
</AdminAccessGate>
