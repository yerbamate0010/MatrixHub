<script lang="ts">
	import { Spinner } from '$lib/components';
	import FeatureHelpModal from '$lib/components/help/FeatureHelpModal.svelte';
	import HelpTriggerButton from '$lib/components/help/HelpTriggerButton.svelte';
	import BaseCard from '$lib/components/layout/BaseCard.svelte';
	import { FormToggle, FormButton, FormInput } from '$lib/components/shared/forms';
	import ContentBox from '$lib/components/layout/ContentBox.svelte';
	import StatusRow from '$lib/components/layout/StatusRow.svelte';
	import { slide } from 'svelte/transition';
	import { cubicOut } from 'svelte/easing';
	import PowerIcon from '~icons/tabler/power';
	import Timer from '~icons/tabler/hourglass';
	import Interval from '~icons/tabler/repeat';
	import Save from '~icons/tabler/device-floppy';
	import type { PowerStatus } from '$lib/types/system/power';
	import { formatMs } from '$lib/features/system/power/formatPowerDuration';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';
	import { createFeatureLinks, powerHelpLinkIds } from '$lib/features/navigation/featureRegistry';

	interface Props {
		status: PowerStatus | null;
		loading: boolean;
		error: string | null;
		errors: {
			inactivity_timeout_ms: boolean;
			grace_after_boot_ms: boolean;
		};
		localSleepEnabled: boolean;
		localInactivityTimeoutMs: number;
		localGraceAfterBootMs: number;
		hasChanges: boolean;
		saving: boolean;
		onSave: () => void;
	}

	let {
		status,
		loading,
		error,
		errors,
		localSleepEnabled = $bindable(),
		localInactivityTimeoutMs = $bindable(),
		localGraceAfterBootMs = $bindable(),
		hasChanges,
		saving,
		onSave
	}: Props = $props();

	const statIconClass = 'h-6 w-6 flex-none text-base-content/70';
	const MS_PER_MIN = 60000;
	const INACTIVITY_MIN_MINUTES = 5;
	const INACTIVITY_MAX_MINUTES = 1440;
	const GRACE_MIN_MINUTES = 1;
	const GRACE_MAX_MINUTES = 10;

	function clampMinutes(value: number, min: number, max: number): number {
		if (!Number.isFinite(value)) return min;
		return Math.min(max, Math.max(min, Math.round(value)));
	}

	function toMinutes(ms: number): number {
		return Math.max(0, Math.round(ms / MS_PER_MIN));
	}

	function toMs(minutes: number): number {
		return Math.max(0, Math.round(minutes)) * MS_PER_MIN;
	}

	let inactivityMinutes = $state(0);
	let graceMinutes = $state(0);
	let editingInactivity = $state(false);
	let editingGrace = $state(false);

	$effect(() => {
		if (!editingInactivity) {
			const next = toMinutes(localInactivityTimeoutMs);
			if (inactivityMinutes !== next) inactivityMinutes = next;
		}
	});

	$effect(() => {
		if (!editingGrace) {
			const next = toMinutes(localGraceAfterBootMs);
			if (graceMinutes !== next) graceMinutes = next;
		}
	});

	const inactivityInvalid = $derived(
		!Number.isFinite(inactivityMinutes) ||
			inactivityMinutes < INACTIVITY_MIN_MINUTES ||
			inactivityMinutes > INACTIVITY_MAX_MINUTES
	);
	const graceInvalid = $derived(
		!Number.isFinite(graceMinutes) ||
			graceMinutes < GRACE_MIN_MINUTES ||
			graceMinutes > GRACE_MAX_MINUTES
	);
	const hasValidationErrors = $derived(inactivityInvalid || graceInvalid);

	function applyInactivityMinutes(next: number) {
		inactivityMinutes = next;
		if (!Number.isFinite(next)) return;
		if (next < INACTIVITY_MIN_MINUTES || next > INACTIVITY_MAX_MINUTES) return;
		localInactivityTimeoutMs = toMs(next);
	}

	function applyGraceMinutes(next: number) {
		graceMinutes = next;
		if (!Number.isFinite(next)) return;
		if (next < GRACE_MIN_MINUTES || next > GRACE_MAX_MINUTES) return;
		localGraceAfterBootMs = toMs(next);
	}

	function clampInactivityOnBlur() {
		editingInactivity = false;
		const clamped = clampMinutes(inactivityMinutes, INACTIVITY_MIN_MINUTES, INACTIVITY_MAX_MINUTES);
		inactivityMinutes = clamped;
		localInactivityTimeoutMs = toMs(clamped);
	}

	function clampGraceOnBlur() {
		editingGrace = false;
		const clamped = clampMinutes(graceMinutes, GRACE_MIN_MINUTES, GRACE_MAX_MINUTES);
		graceMinutes = clamped;
		localGraceAfterBootMs = toMs(clamped);
	}

	function handleSubmit(event: Event) {
		event.preventDefault();
		clampInactivityOnBlur();
		clampGraceOnBlur();
		onSave();
	}

	let helpOpen = $state(false);
	const locale = $derived(i18n.languageTag);
	const helpSections = $derived([
		{
			title: m.help_modal_how_title({ locale }),
			body: m.power_help_how_body({ locale })
		},
		{
			title: m.help_modal_setup_title({ locale }),
			body: m.power_help_setup_body({ locale })
		},
		{
			title: m.help_modal_watch_title({ locale }),
			body: m.power_help_watch_body({ locale })
		}
	]);
	const helpLinks = $derived(createFeatureLinks(powerHelpLinkIds, locale));
</script>

<BaseCard title={m.power_config_title({ locale: i18n.languageTag })} icon={PowerIcon}>
	{#snippet actions()}
		<HelpTriggerButton label={m.power_help_title({ locale })} onclick={() => (helpOpen = true)} />
	{/snippet}

	{#if loading}
		<div class="flex justify-center items-center py-8">
			<Spinner />
		</div>
	{:else if error}
		<div class="alert alert-error text-sm">
			{m.power_config_unavailable({ locale: i18n.languageTag })}
		</div>
	{:else if status}
		<form
			onsubmit={handleSubmit}
			class="flex w-full flex-col gap-1"
			transition:slide|local={{ duration: 300, easing: cubicOut }}
			novalidate
		>
			<FormToggle
				label={m.power_autosleep_title({ locale: i18n.languageTag })}
				description={localSleepEnabled
					? m.power_autosleep_desc_on(
							{ time: formatMs(localInactivityTimeoutMs) },
							{ locale: i18n.languageTag }
						)
					: m.power_autosleep_desc_off({ locale: i18n.languageTag })}
				bind:checked={localSleepEnabled}
			/>

			<ContentBox>
				<FormInput
					label={m.power_inactivity_title(
						{ unit: m.unit_min({ locale: i18n.languageTag }) },
						{ locale: i18n.languageTag }
					)}
					type="number"
					min={INACTIVITY_MIN_MINUTES}
					max={INACTIVITY_MAX_MINUTES}
					step={1}
					value={inactivityMinutes}
					error={inactivityInvalid || errors.inactivity_timeout_ms
						? m.power_inactivity_invalid_range(
								{
									min: INACTIVITY_MIN_MINUTES,
									max: INACTIVITY_MAX_MINUTES,
									unit: m.unit_min({ locale: i18n.languageTag })
								},
								{ locale: i18n.languageTag }
							)
						: undefined}
					onfocus={() => {
						editingInactivity = true;
					}}
					oninput={(e) => {
						const next = Number((e.target as HTMLInputElement).value);
						applyInactivityMinutes(next);
					}}
					onblur={clampInactivityOnBlur}
				>
					{#snippet suffix()}
						<span class="text-xs text-base-content/60">
							{m.unit_min({ locale: i18n.languageTag })}
						</span>
					{/snippet}
				</FormInput>
			</ContentBox>

			<ContentBox>
				<FormInput
					label={m.power_grace_title(
						{ unit: m.unit_min({ locale: i18n.languageTag }) },
						{ locale: i18n.languageTag }
					)}
					type="number"
					min={GRACE_MIN_MINUTES}
					max={GRACE_MAX_MINUTES}
					step={1}
					value={graceMinutes}
					error={graceInvalid || errors.grace_after_boot_ms
						? m.power_grace_invalid_range(
								{
									min: GRACE_MIN_MINUTES,
									max: GRACE_MAX_MINUTES,
									unit: m.unit_min({ locale: i18n.languageTag })
								},
								{ locale: i18n.languageTag }
							)
						: undefined}
					onfocus={() => {
						editingGrace = true;
					}}
					oninput={(e) => {
						const next = Number((e.target as HTMLInputElement).value);
						applyGraceMinutes(next);
					}}
					onblur={clampGraceOnBlur}
				>
					{#snippet suffix()}
						<span class="text-xs text-base-content/60">
							{m.unit_min({ locale: i18n.languageTag })}
						</span>
					{/snippet}
				</FormInput>
			</ContentBox>

			<StatusRow
				icon={Interval}
				iconClass={statIconClass}
				label={m.power_wake_title({ locale: i18n.languageTag })}
				value={formatMs(status.wake_interval_ms)}
			/>

			{#if status.sleep_requested}
				<StatusRow
					icon={Timer}
					iconClass={statIconClass}
					label={m.power_sleep_eta({ locale: i18n.languageTag })}
					value={formatMs(status.sleep_eta_ms)}
					valueClass="text-sm text-warning font-semibold"
				/>
			{/if}
			<div class="flex justify-end mt-4">
				<FormButton
					type="submit"
					label={m.action_save({ locale: i18n.languageTag })}
					icon={Save}
					disabled={!hasChanges || saving || hasValidationErrors}
					loading={saving}
				/>
			</div>
		</form>
	{/if}

	<FeatureHelpModal
		isOpen={helpOpen}
		onClose={() => (helpOpen = false)}
		title={m.power_help_title({ locale })}
		intro={m.power_help_intro({ locale })}
		sections={helpSections}
		links={helpLinks}
	/>
</BaseCard>
