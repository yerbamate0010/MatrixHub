<script lang="ts">
	import { onMount } from 'svelte';
	import FeatureHelpModal from '$lib/components/help/FeatureHelpModal.svelte';
	import HelpTriggerButton from '$lib/components/help/HelpTriggerButton.svelte';
	import { loadUplotLib } from '$lib/utils/charts/useUplotChart.svelte';
	import ChartLine from '~icons/tabler/chart-line';
	import Wifi from '~icons/tabler/wifi';
	import Antenna from '~icons/tabler/antenna';
	import { Spinner } from '$lib/components';
	import BaseCard from '$lib/components/layout/BaseCard.svelte';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';
	import { useCsiAmplitudeChart } from './useCsiAmplitudeChart.svelte';

	interface Props {
		amplitudes: Float32Array;
		subcarriers: number;
		rssi: number;
		gain: number;
		isConnected: boolean;
		userEnabled: boolean;
		onToggleConnection: () => void;
	}

	let { amplitudes, subcarriers, rssi, gain, isConnected, userEnabled, onToggleConnection }: Props =
		$props();

	let canvasAmp: HTMLDivElement;
	let chart = $state<ReturnType<typeof useCsiAmplitudeChart> | undefined>(undefined);
	let isLoading = $state(true);
	let helpOpen = $state(false);
	const locale = $derived(i18n.languageTag);
	const streamStateLabel = $derived(
		isConnected
			? m.csi_status_live()
			: userEnabled
				? m.csi_status_connecting()
				: m.csi_status_paused()
	);
	const streamStateClass = $derived(
		isConnected ? 'badge-success' : userEnabled ? 'badge-warning' : 'badge-ghost'
	);
	const helpSections = $derived([
		{
			title: m.help_modal_how_title({ locale }),
			body: m.csi_help_how_body({ locale })
		},
		{
			title: m.help_modal_setup_title({ locale }),
			body: m.csi_help_setup_body({ locale })
		},
		{
			title: m.help_modal_watch_title({ locale }),
			body: m.csi_help_watch_body({ locale })
		}
	]);
	const helpLinks = $derived([
		{ href: '/wifisensing', label: m.menu_wifisensing({ locale }) },
		{ href: '/system/help', label: m.menu_help({ locale }) }
	]);

	// Watch for data updates
	$effect(() => {
		if (amplitudes.length > 0 && chart) {
			chart.update(amplitudes);
		}
	});

	onMount(() => {
		let isDisposed = false;

		void (async () => {
			try {
				const uPlot = await loadUplotLib();
				if (!uPlot || isDisposed) return;

				chart = useCsiAmplitudeChart(() => amplitudes, { mount: canvasAmp }, uPlot);
				chart.render();
				chart.setupResizeObserver();
			} finally {
				if (!isDisposed) {
					isLoading = false;
				}
			}
		})();

		return () => {
			isDisposed = true;
			chart?.destroy();
			chart = undefined;
		};
	});
</script>

<BaseCard title={m.csi_amplitudes_title()} icon={ChartLine} class="h-full">
	{#snippet actions()}
		<div class="flex flex-wrap gap-2 sm:gap-4 text-xs items-center justify-end mr-1">
			<HelpTriggerButton label={m.csi_help_title({ locale })} onclick={() => (helpOpen = true)} />

			<div class="hidden xs:flex gap-2 font-mono border-r border-base-content/20 pr-2 sm:pr-4 mr-2">
				<div class="hidden sm:block">
					{m.csi_index_label()}:
					<span class="font-bold inline-block w-[3ch] text-right">{chart?.legendIndex ?? '--'}</span
					>
				</div>
				<div class="flex items-center gap-1">
					<div class="w-2 h-2 rounded-full bg-[#00ba55]"></div>
					<span class="hidden sm:inline">{m.csi_amplitude_short_label()}:</span><span
						class="font-bold w-[4ch] sm:w-[5ch] text-right">{chart?.legendAmp ?? '--'}</span
					>
				</div>
			</div>

			<div class="hidden lg:flex items-center gap-3 border-r border-base-content/20 pr-4 mr-2">
				<div class="flex items-center gap-1" title={m.csi_signal_strength_title()}>
					<Antenna class="w-4 h-4 opacity-70" />
					<span class="font-mono">{rssi}dBm</span>
				</div>
				<div class="opacity-50 font-mono text-[10px]">
					{m.csi_gain_label()}:{gain.toFixed(1)}
				</div>
				<div class="opacity-50 font-mono text-[10px]">
					{subcarriers}{m.csi_subcarriers_suffix()}
				</div>
				<div class="badge badge-xs {streamStateClass} uppercase">{streamStateLabel}</div>
			</div>

			<div class="flex items-center gap-2" title={m.csi_toggle_connection_title()}>
				<Wifi class="w-4 h-4 {isConnected ? 'text-success' : 'opacity-50'}" />
				<input
					type="checkbox"
					class="toggle toggle-success toggle-sm"
					checked={userEnabled}
					aria-label={m.csi_toggle_connection_title()}
					onchange={onToggleConnection}
				/>
			</div>
		</div>
	{/snippet}
	<div bind:this={canvasAmp} class="relative w-full h-[190px]">
		{#if isLoading}
			<div
				class="absolute inset-0 flex items-center justify-center bg-base-100/30 z-20 backdrop-blur-[1px]"
			>
				<Spinner />
			</div>
		{/if}
	</div>

	<FeatureHelpModal
		isOpen={helpOpen}
		onClose={() => (helpOpen = false)}
		title={m.csi_help_title({ locale })}
		intro={m.csi_help_intro({ locale })}
		sections={helpSections}
		links={helpLinks}
	/>
</BaseCard>
