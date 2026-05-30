<script lang="ts">
	import { PageWrapper, GridLayout } from '$lib/components/layout';
	import CsiAmplitudeChart from '$lib/features/wifisensing/csi/CsiAmplitudeChart.svelte';
	import CsiWaterfallChart from '$lib/features/wifisensing/csi/CsiWaterfallChart.svelte';
	import { useCsiConnection } from '$lib/features/wifisensing/csi/useCsiConnection.svelte';

	const csi = useCsiConnection();
</script>

<PageWrapper>
	<GridLayout cols={1}>
		<div class="flex flex-col gap-4">
			<CsiAmplitudeChart
				amplitudes={csi.amplitudes}
				subcarriers={csi.subcarriers}
				rssi={csi.rssi}
				gain={csi.gain}
				isConnected={csi.isConnected}
				userEnabled={csi.userEnabled}
				onToggleConnection={() => csi.toggle()}
			/>

			<CsiWaterfallChart
				amplitudes={csi.amplitudes}
				subcarriers={csi.subcarriers}
				timestamp={csi.timestamp}
				fps={csi.fps}
			/>
		</div>
	</GridLayout>
</PageWrapper>
