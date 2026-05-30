<script lang="ts">
	import { onDestroy, onMount } from 'svelte';
	import { useSystemTransportManagement } from '$lib/features/system/status/useSystemTransportManagement.svelte';
	import { setGlobalUnauthorizedHandler } from '$lib/utils/api/apiClient';
	import type { SessionAccess } from '$lib/features/auth/useSessionAccess.svelte';
	import Menu from './menu.svelte';
	import Statusbar from './statusbar.svelte';

	interface Props {
		session: SessionAccess;
		children?: import('svelte').Snippet;
	}

	let { session, children }: Props = $props();
	const getSession = () => session;
	const systemTransport = useSystemTransportManagement();

	// Once the app shell is mounted, any backend 401 should collapse back to the
	// login screen through one shared unauthorized path.
	setGlobalUnauthorizedHandler(() => getSession().invalidate('unauthorized'));

	let menuOpen = $state(false);

	onMount(() => {
		// The authenticated shell owns the core system_status lease so the topbar
		// and other shell-level UI do not depend on whichever page happens to be open.
		systemTransport.subscribeChannel('system_status');
	});

	onDestroy(() => {
		systemTransport.unsubscribeChannel('system_status');
	});
</script>

<div class="drawer lg:drawer-open w-full max-w-full overflow-x-hidden">
	<input id="main-menu" type="checkbox" class="drawer-toggle" bind:checked={menuOpen} />
	<div class="drawer-content flex w-full max-w-full flex-col">
		<Statusbar />
		{@render children?.()}
	</div>
	<div class="drawer-side z-30 shadow-lg">
		<label for="main-menu" class="drawer-overlay"></label>
		<Menu
			closeMenu={() => {
				menuOpen = false;
			}}
		/>
	</div>
</div>
