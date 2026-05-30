<script lang="ts">
	import { Spinner } from '$lib/components';
	import BaseCard from '$lib/components/layout/BaseCard.svelte';
	import { FormButton } from '$lib/components/shared/forms';
	import Mouse from '~icons/tabler/mouse';
	import Save from '~icons/tabler/device-floppy';
	import HandClick from '~icons/tabler/hand-click';
	import Pointer from '~icons/tabler/pointer';
	import Help from '~icons/tabler/help';
	import * as m from '$lib/paraglide/messages.js';
	import { AIR_MOUSE_CLICK_SOURCE } from './airMouseConfig';
	import AirMouseSettings from './components/AirMouseSettings.svelte';
	import AirMouseStatus from './components/AirMouseStatus.svelte';
	import AirMouseClickHelpModal from './components/AirMouseClickHelpModal.svelte';
	import AirMouseCursorHelpModal from './components/AirMouseCursorHelpModal.svelte';
	import { useAirMouseSettingsForm } from './useAirMouseSettingsForm.svelte';

	import type { AirMouseState } from './airMouseState.svelte';
	import { fade } from 'svelte/transition';
	import { onDestroy } from 'svelte';

	let {
		mouseState
	}: {
		mouseState: AirMouseState;
	} = $props();

	const form = useAirMouseSettingsForm(() => mouseState);
	let showClickHelp = $state(false);
	let showCursorHelp = $state(false);

	// Computed icon class based on state
	const iconClass = $derived.by(() => {
		if (!mouseState.status) return '';
		const stateClass = mouseState.status?.running ? 'text-success' : 'text-warning';
		const pulseClass = mouseState.wsConnected ? 'animate-pulse' : '';
		return `${stateClass} ${pulseClass}`;
	});

	// WS + live status should follow backend state (not unsaved local edits)
	const needsImu = $derived.by(() => {
		const s = mouseState.status;
		if (!s) return false;
		return (
			s.movement_enabled || (s.click_enabled && s.click_source === AIR_MOUSE_CLICK_SOURCE.SENSOR)
		);
	});

	$effect(() => {
		if (typeof document === 'undefined') return;
		needsImu;
		mouseState.uiRequestingConnection = needsImu;
		mouseState.updateConnectionState();
	});

	onDestroy(() => {
		if (typeof document === 'undefined') return;
		mouseState.uiRequestingConnection = false;
		mouseState.updateConnectionState();
	});
</script>

<BaseCard
	title={m.airmouse_title()}
	icon={Mouse}
	iconClass={`h-6 w-6 ${iconClass}`}
	class="mb-4"
	hideTitleOnTiny={false}
>
	{#if !mouseState.status}
		<div class="flex justify-center items-center py-8">
			<Spinner />
		</div>
	{:else if needsImu}
		<div in:fade>
			<AirMouseStatus {mouseState} />
		</div>
	{:else}
		<div class="py-6 text-center opacity-60 text-sm">
			{m.airmouse_enable_hint()}
		</div>
	{/if}
</BaseCard>

<div class="grid grid-cols-1 md:grid-cols-2 gap-4">
	<!-- Left card: Click settings -->
	<div class="order-2 md:order-1 h-full">
		<BaseCard
			title={m.airmouse_click_title()}
			icon={HandClick}
			class="h-full"
			hideTitleOnTiny={false}
		>
			{#snippet actions()}
				<FormButton
					variant="icon"
					size="sm"
					icon={Help}
					ariaLabel={m.airmouse_click_help_btn()}
					title={m.airmouse_click_help_btn()}
					onclick={(e) => {
						e.stopPropagation();
						showClickHelp = true;
					}}
				/>
			{/snippet}

			{#if mouseState.status}
				<AirMouseSettings
					bind:settings={form.settings}
					scripts={mouseState.scripts}
					{mouseState}
					section="click"
					showHeader={false}
				/>
			{:else}
				<div class="flex justify-center items-center py-8">
					<Spinner />
				</div>
			{/if}
		</BaseCard>
	</div>

	<!-- Right card: Cursor settings -->
	<div class="order-1 md:order-2 h-full">
		<BaseCard
			title={m.airmouse_cursor_title()}
			icon={Pointer}
			class="h-full"
			hideTitleOnTiny={false}
		>
			{#snippet actions()}
				<FormButton
					variant="icon"
					size="sm"
					icon={Help}
					ariaLabel={m.airmouse_cursor_help_btn()}
					title={m.airmouse_cursor_help_btn()}
					onclick={(e) => {
						e.stopPropagation();
						showCursorHelp = true;
					}}
				/>
			{/snippet}

			{#if mouseState.status}
				<div class="flex flex-col gap-1 h-full">
					<AirMouseSettings
						bind:settings={form.settings}
						scripts={mouseState.scripts}
						{mouseState}
						section="cursor"
						showHeader={false}
					/>

					<div class="flex justify-end mt-auto pt-2">
						<FormButton
							variant="primary"
							label={m.action_save()}
							icon={Save}
							onclick={form.confirmSave}
							disabled={form.saving || !form.hasChanges}
							loading={form.saving}
						/>
					</div>
				</div>
			{:else}
				<div class="flex justify-center items-center py-8">
					<Spinner />
				</div>
			{/if}
		</BaseCard>
	</div>
</div>

<AirMouseClickHelpModal bind:isOpen={showClickHelp} />
<AirMouseCursorHelpModal bind:isOpen={showCursorHelp} />
