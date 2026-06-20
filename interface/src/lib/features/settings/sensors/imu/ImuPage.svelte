<script lang="ts">
	import Activity from '~icons/tabler/activity';
	import Gauge from '~icons/tabler/gauge';
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
	import { useImuSettings } from './useImuSettings.svelte';

	const session = useSessionAccess();
	const canManage = $derived(session.canManage);
	const imu = useImuSettings({ shouldLoad: () => canManage });

	const status = $derived(imu.status);
	const metrics = $derived(status?.metrics ?? null);
	const consumerRows = $derived.by(() => {
		const consumers = status?.consumers;
		return [
			{ key: 'ui_monitor', label: 'UI monitor', data: consumers?.ui_monitor },
			{ key: 'alarm', label: 'Alarm', data: consumers?.alarm },
			{ key: 'auto_rotate', label: 'Matrix auto-rotate', data: consumers?.auto_rotate },
			{ key: 'airmouse_movement', label: 'AirMouse movement', data: consumers?.airmouse_movement },
			{ key: 'airmouse_click', label: 'AirMouse click', data: consumers?.airmouse_click }
		];
	});

	function yesNo(value?: boolean) {
		return value ? 'OK' : '--';
	}

	function freshLabel(value?: boolean) {
		return value ? 'fresh' : 'stale';
	}

	function numberLabel(value: number | null | undefined, digits = 2, suffix = '') {
		if (value === null || value === undefined || Number.isNaN(value)) return '--';
		return `${value.toFixed(digits)}${suffix}`;
	}

	function vectorLabel(vector: { x: number; y: number; z: number } | undefined) {
		if (!vector) return '--';
		return `${vector.x.toFixed(3)}, ${vector.y.toFixed(3)}, ${vector.z.toFixed(3)}`;
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
						subValue={`${status?.sample_age_ms ?? 0} ms`}
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
				</div>
			{/if}
		</BaseCard>

		<BaseCard title={m.imu_consumers({ locale: i18n.languageTag })} icon={Rotate} class="h-full">
			<div class="space-y-2">
				{#each consumerRows as row (row.key)}
					<StatusRow
						icon={Activity}
						label={row.label}
						value={row.data?.desired ? 'desired' : 'off'}
						subValue={row.data?.running ? 'running' : 'idle'}
					/>
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
				<div class="flex justify-end pt-2">
					<FormButton
						icon={Target}
						label={m.imu_calibrate_orientation({ locale: i18n.languageTag })}
						loading={imu.calibrating}
						disabled={imu.calibrating}
						onclick={imu.calibrateOrientation}
					/>
				</div>
			</div>
		</BaseCard>
	</GridLayout>
</AdminAccessGate>
