<script lang="ts">
	import { onMount } from 'svelte';
	import BaseCard from '$lib/components/layout/BaseCard.svelte';
	import * as m from '$lib/paraglide/messages.js';
	import { formatCsiTimestamp } from './csiWaterfallRaster';
	import { useCsiWaterfallChart } from './useCsiWaterfallChart.svelte';

	interface Props {
		amplitudes: Float32Array;
		subcarriers: number;
		timestamp: number;
		fps: number;
	}

	let { amplitudes, subcarriers, timestamp, fps }: Props = $props();

	let canvasWater: HTMLCanvasElement;
	const chart = useCsiWaterfallChart(() => amplitudes, { getCanvas: () => canvasWater });
	const gridLines = $derived.by(() => {
		const ticks: number[] = [];
		const carrierCount = Math.max(subcarriers || amplitudes.length, 1);

		for (let tick = 0; tick < carrierCount; tick += 20) {
			ticks.push(tick);
		}

		return ticks;
	});

	$effect(() => {
		if (amplitudes.length > 0 && canvasWater) {
			chart.update(amplitudes);
		}
	});

	onMount(() => {
		chart.init();
		return () => chart.destroy();
	});
</script>

<BaseCard title={m.csi_waterfall_title()}>
	{#snippet actions()}
		<div class="flex items-center gap-2 sm:gap-4 text-xs font-mono opacity-80 mr-2">
			<div class="flex items-center">
				<span class="hidden sm:inline">{m.csi_time_label()}:&nbsp;</span>
				<span class="font-bold inline-block min-w-[12ch]">{formatCsiTimestamp(timestamp)}</span>
			</div>
			<div class="flex items-center">
				<span>{m.csi_fps_label()}:&nbsp;</span>
				<span class="text-info font-bold inline-block min-w-[3ch] text-right">{fps}</span>
			</div>
		</div>
	{/snippet}
	<div class="relative w-full h-[300px] bg-black rounded overflow-hidden">
		<canvas bind:this={canvasWater} class="w-full h-full block"></canvas>

		{#each gridLines as tick}
			<div
				class="absolute top-0 bottom-0 w-[1px] bg-[#333] pointer-events-none"
				style="left: {((tick + 0.5) / Math.max(subcarriers || amplitudes.length, 1)) * 100}%"
			></div>
		{/each}
	</div>
</BaseCard>
