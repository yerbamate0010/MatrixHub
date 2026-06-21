<script lang="ts">
	import MatrixSectionPage from './MatrixSectionPage.svelte';
	import MatrixSettings from './MatrixSettings.svelte';
	import { MATRIX_DISPLAY_SETTING_KEYS } from './matrixModel';
</script>

<MatrixSectionPage trackedKeys={MATRIX_DISPLAY_SETTING_KEYS}>
	{#snippet children(store, canManage)}
		<MatrixSettings {store} {canManage} />
	{/snippet}
</MatrixSectionPage>
