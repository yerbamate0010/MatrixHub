<script lang="ts">
	import { MatrixApiService } from '$lib/services/api/core/MatrixApiService';
	import { useSessionAccess } from '$lib/features/auth/useSessionAccess.svelte';
	import Lock from '~icons/tabler/lock';
	import { PageWrapper, GridLayout } from '$lib/components/layout';
	import * as m from '$lib/paraglide/messages.js';
	import MatrixSettings from './MatrixSettings.svelte';
	import MatrixEffects from './MatrixEffects.svelte';
	import { useMatrixSettings } from './useMatrixSettings.svelte';

	const session = useSessionAccess();
	const matrixState = useMatrixSettings(() => new MatrixApiService(session.apiOptions), {
		shouldLoad: () => canRead
	});
	const canRead = $derived(session.canRead);
	const canManage = $derived(session.canManage);
</script>

<PageWrapper>
	{#if !canRead}
		<GridLayout cols={2}>
			<div
				class="rounded-md bg-base-200 p-8 flex flex-col items-center justify-center text-center gap-3 col-span-1"
			>
				<div class="p-3 bg-base-100 rounded-full">
					<Lock class="h-6 w-6 text-base-content/50" />
				</div>
				<div class="flex flex-col gap-1">
					<h4 class="font-medium">{m.matrix_admin_title()}</h4>
					<p class="text-sm text-base-content/70 max-w-sm">
						{m.matrix_admin_desc()}
					</p>
				</div>
			</div>
		</GridLayout>
	{:else}
		<GridLayout cols={2}>
			<MatrixSettings store={matrixState} {canManage} />
			<MatrixEffects store={matrixState} {canManage} />
		</GridLayout>
	{/if}
</PageWrapper>
