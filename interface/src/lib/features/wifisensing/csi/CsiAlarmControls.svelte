<script lang="ts">
	import BaseCard from '$lib/components/layout/BaseCard.svelte';
	import type {
		CsiAlarmBand,
		CsiAlarmSettings,
		CsiMotionStatus
	} from '$lib/types/connectivity/wifiSensing';
	import Bell from '~icons/tabler/bell-ringing';
	import Plus from '~icons/tabler/plus';
	import Refresh from '~icons/tabler/refresh';
	import Save from '~icons/tabler/device-floppy';
	import Target from '~icons/tabler/target';
	import Trash from '~icons/tabler/trash';
	import X from '~icons/tabler/x';
	import * as m from '$lib/paraglide/messages.js';

	interface Props {
		settings: CsiAlarmSettings;
		status: CsiMotionStatus | null;
		isAdmin: boolean;
		hasChanges: boolean;
		saving: boolean;
		calibrating: boolean;
		selectionMode?: boolean;
		subcarriers: number;
		onSave: () => void;
		onReset: () => void;
		onCalibrate: () => void;
		onAddBand: () => void;
		onRemoveBand: (index: number) => void;
		onSensitivity: (value: 0 | 1 | 2) => void;
	}

	let {
		settings,
		status,
		isAdmin,
		hasChanges,
		saving,
		calibrating,
		selectionMode = $bindable(false),
		subcarriers,
		onSave,
		onReset,
		onCalibrate,
		onAddBand,
		onRemoveBand,
		onSensitivity
	}: Props = $props();

	const stateLabel = $derived.by(() => {
		if (!settings.enabled) return m.csi_alarm_state_disabled();
		if (!status) return m.csi_alarm_state_unknown();
		if (status.detected) return m.csi_alarm_state_motion();
		if (status.noisy) return m.csi_alarm_state_noisy();
		if (status.state === 'calibrating') return m.csi_alarm_state_calibrating();
		if (status.state === 'needs_configuration') return m.csi_alarm_state_needs_configuration();
		if (status.baseline_ready) return m.csi_alarm_state_monitoring();
		return status.state.replaceAll('_', ' ');
	});
	const stateClass = $derived(
		status?.detected
			? 'badge-error'
			: status?.noisy
				? 'badge-warning'
				: settings.enabled
					? 'badge-success'
					: 'badge-ghost'
	);
	const carrierCount = $derived(Math.max(subcarriers || status?.width || 256, 1));
	const selectionLabel = $derived(
		selectionMode ? m.csi_alarm_selecting() : m.csi_alarm_select_band()
	);

	function normalizeBand(band: CsiAlarmBand) {
		const start = Math.max(0, Math.min(255, Math.round(Number(band.start) || 0)));
		const end = Math.max(0, Math.min(255, Math.round(Number(band.end) || 0)));
		if (start <= end) {
			band.start = start;
			band.end = end;
		} else {
			band.start = end;
			band.end = start;
		}
	}

	function updateSensitivity(event: Event) {
		const value = Number((event.currentTarget as HTMLSelectElement).value);
		onSensitivity((Number.isFinite(value) ? value : 1) as 0 | 1 | 2);
	}
</script>

<BaseCard title={m.csi_alarm_title()} icon={Bell}>
	{#snippet actions()}
		<div class="flex items-center gap-2">
			<div class="badge badge-sm {stateClass}">{stateLabel}</div>
			<label class="flex items-center gap-2">
				<input
					type="checkbox"
					class="toggle toggle-primary toggle-sm"
					bind:checked={settings.enabled}
					disabled={!isAdmin}
					aria-label={m.csi_alarm_enabled()}
				/>
			</label>
		</div>
	{/snippet}

	<div class="grid gap-4 lg:grid-cols-[1fr_auto]">
		<div class="grid gap-3 sm:grid-cols-3">
			<div class="rounded-md border border-base-300/60 bg-base-100/30 px-3 py-2">
				<div class="text-[10px] uppercase opacity-60">{m.csi_alarm_score()}</div>
				<div class="font-mono text-lg">{(status?.score ?? 0).toFixed(2)}</div>
			</div>
			<div class="rounded-md border border-base-300/60 bg-base-100/30 px-3 py-2">
				<div class="text-[10px] uppercase opacity-60">{m.csi_alarm_baseline()}</div>
				<div class="font-mono text-lg">{status?.baseline_ready ? 'ready' : 'wait'}</div>
			</div>
			<div class="rounded-md border border-base-300/60 bg-base-100/30 px-3 py-2">
				<div class="text-[10px] uppercase opacity-60">{m.csi_alarm_carriers()}</div>
				<div class="font-mono text-lg">
					{status?.valid_carriers ?? 0}/{status?.selected_carriers ?? 0}
				</div>
			</div>
		</div>

		<div class="flex flex-wrap items-start gap-2 lg:justify-end">
			<button
				type="button"
				class="btn btn-sm btn-outline"
				class:btn-active={selectionMode}
				disabled={!isAdmin || !settings.enabled}
				onclick={() => (selectionMode = !selectionMode)}
			>
				<Target class="h-4 w-4" />
				{selectionLabel}
			</button>
			<button
				type="button"
				class="btn btn-sm btn-outline"
				disabled={!isAdmin || settings.bands.length >= 4}
				onclick={onAddBand}
				title={m.csi_alarm_add_band()}
			>
				<Plus class="h-4 w-4" />
				{m.csi_alarm_add_band()}
			</button>
			<button
				type="button"
				class="btn btn-sm btn-outline"
				disabled={!isAdmin || calibrating || !settings.enabled}
				onclick={onCalibrate}
			>
				<Refresh class="h-4 w-4" />
				{calibrating ? m.common_loading() : m.csi_alarm_calibrate()}
			</button>
		</div>
	</div>

	<div class="mt-4 flex flex-wrap gap-2">
		{#each settings.bands as band, index}
			<div class="join">
				<label class="join-item input input-bordered input-sm flex items-center gap-1">
					<span class="text-[10px] opacity-60">S</span>
					<input
						type="number"
						class="w-14 bg-transparent outline-none"
						min="0"
						max={carrierCount - 1}
						bind:value={band.start}
						disabled={!isAdmin}
						onchange={() => normalizeBand(band)}
					/>
				</label>
				<label class="join-item input input-bordered input-sm flex items-center gap-1">
					<span class="text-[10px] opacity-60">E</span>
					<input
						type="number"
						class="w-14 bg-transparent outline-none"
						min="0"
						max={carrierCount - 1}
						bind:value={band.end}
						disabled={!isAdmin}
						onchange={() => normalizeBand(band)}
					/>
				</label>
				<button
					type="button"
					class="btn btn-sm btn-outline join-item"
					disabled={!isAdmin}
					onclick={() => onRemoveBand(index)}
					title={m.action_delete()}
				>
					<Trash class="h-4 w-4" />
				</button>
			</div>
		{:else}
			<div class="badge badge-outline">{m.csi_alarm_no_bands()}</div>
		{/each}
	</div>

	<div class="mt-4 grid gap-3 sm:grid-cols-2 lg:grid-cols-4">
		<label class="form-control">
			<span class="label-text text-xs">{m.csi_alarm_sensitivity()}</span>
			<select
				class="select select-bordered select-sm"
				value={settings.sensitivity}
				disabled={!isAdmin}
				onchange={updateSensitivity}
			>
				<option value="0">{m.csi_alarm_sensitivity_low()}</option>
				<option value="1">{m.csi_alarm_sensitivity_medium()}</option>
				<option value="2">{m.csi_alarm_sensitivity_high()}</option>
			</select>
		</label>
		<label class="form-control">
			<span class="label-text text-xs">{m.csi_alarm_enter_threshold()}</span>
			<input
				type="number"
				class="input input-bordered input-sm"
				step="0.1"
				min="1"
				max="100"
				bind:value={settings.enter_threshold}
				readonly
				aria-readonly="true"
				disabled={!isAdmin}
			/>
		</label>
		<label class="form-control">
			<span class="label-text text-xs">{m.csi_alarm_clear_threshold()}</span>
			<input
				type="number"
				class="input input-bordered input-sm"
				step="0.1"
				min="0.5"
				max={settings.enter_threshold}
				bind:value={settings.clear_threshold}
				readonly
				aria-readonly="true"
				disabled={!isAdmin}
			/>
		</label>
		<label class="flex items-end gap-2 rounded-md border border-base-300/60 px-3 py-2">
			<input
				type="checkbox"
				class="checkbox checkbox-sm"
				bind:checked={settings.auto_recalibration}
				disabled={!isAdmin}
			/>
			<span class="text-xs">{m.csi_alarm_auto_recalibration()}</span>
		</label>
	</div>

	<details class="mt-4">
		<summary class="cursor-pointer text-sm font-semibold">{m.csi_alarm_advanced()}</summary>
		<div class="mt-3 grid gap-3 sm:grid-cols-2 lg:grid-cols-4">
			<label class="form-control">
				<span class="label-text text-xs">{m.csi_alarm_top_k()}</span>
				<input
					type="number"
					class="input input-bordered input-sm"
					min="1"
					max="32"
					bind:value={settings.top_k}
					disabled={!isAdmin}
				/>
			</label>
			<label class="form-control">
				<span class="label-text text-xs">{m.csi_alarm_baseline_frames()}</span>
				<input
					type="number"
					class="input input-bordered input-sm"
					min="30"
					max="1000"
					bind:value={settings.baseline_frames}
					disabled={!isAdmin}
				/>
			</label>
			<label class="form-control">
				<span class="label-text text-xs">{m.csi_alarm_hold_ms()}</span>
				<input
					type="number"
					class="input input-bordered input-sm"
					min="100"
					max="10000"
					bind:value={settings.hold_ms}
					disabled={!isAdmin}
				/>
			</label>
			<label class="form-control">
				<span class="label-text text-xs">{m.csi_alarm_clear_hold_ms()}</span>
				<input
					type="number"
					class="input input-bordered input-sm"
					min="100"
					max="30000"
					bind:value={settings.clear_hold_ms}
					disabled={!isAdmin}
				/>
			</label>
			<label class="form-control">
				<span class="label-text text-xs">{m.csi_alarm_min_noise()}</span>
				<input
					type="number"
					class="input input-bordered input-sm"
					step="0.1"
					min="0.1"
					max="1000"
					bind:value={settings.min_noise}
					disabled={!isAdmin}
				/>
			</label>
			<label class="form-control">
				<span class="label-text text-xs">{m.csi_alarm_min_energy()}</span>
				<input
					type="number"
					class="input input-bordered input-sm"
					step="0.1"
					min="0"
					max="10000"
					bind:value={settings.min_energy}
					disabled={!isAdmin}
				/>
			</label>
			<label class="form-control">
				<span class="label-text text-xs">{m.csi_alarm_noisy_threshold()}</span>
				<input
					type="number"
					class="input input-bordered input-sm"
					step="0.1"
					min={settings.enter_threshold}
					max="500"
					bind:value={settings.noisy_threshold}
					disabled={!isAdmin}
				/>
			</label>
		</div>
	</details>

	<div class="mt-4 flex flex-wrap justify-end gap-2">
		<button
			type="button"
			class="btn btn-sm btn-ghost"
			disabled={!isAdmin || !hasChanges || saving}
			onclick={onReset}
		>
			<X class="h-4 w-4" />
			{m.alarms_form_cancel()}
		</button>
		<button
			type="button"
			class="btn btn-sm btn-primary"
			disabled={!isAdmin || !hasChanges || saving}
			onclick={onSave}
		>
			<Save class="h-4 w-4" />
			{saving ? m.common_loading() : m.csi_alarm_save()}
		</button>
	</div>
</BaseCard>
