<script lang="ts">
	import { MatrixApiService } from '$lib/services/api/core/MatrixApiService';
	import { useSessionAccess } from '$lib/features/auth/useSessionAccess.svelte';
	import { PageWrapper, GridLayout } from '$lib/components/layout';
	import MatrixAccessNotice from './MatrixAccessNotice.svelte';
	import MatrixAlarmSettings from './MatrixAlarmSettings.svelte';
	import MatrixSectionTabs from './MatrixSectionTabs.svelte';
	import { useMatrixSettings } from './useMatrixSettings.svelte';
	import { MATRIX_ALARM_SETTING_KEYS } from './matrixModel';

	const session = useSessionAccess();
	const matrixState = useMatrixSettings(() => new MatrixApiService(session.apiOptions), {
		shouldLoad: () => canRead,
		trackedKeys: MATRIX_ALARM_SETTING_KEYS
	});
	const canRead = $derived(session.canRead);
	const canManage = $derived(session.canManage);
</script>

<PageWrapper>
	<MatrixSectionTabs />
	{#if !canRead}
		<GridLayout cols={1}>
			<MatrixAccessNotice />
		</GridLayout>
	{:else}
		<GridLayout cols={1}>
			<MatrixAlarmSettings store={matrixState} {canManage} />
		</GridLayout>
	{/if}
</PageWrapper>
