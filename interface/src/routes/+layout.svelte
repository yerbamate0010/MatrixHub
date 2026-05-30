<script lang="ts">
	import { onMount } from 'svelte';
	import { page } from '$app/state';
	import { Modals } from 'svelte-modals';
	import { appFeatures } from '$lib/stores/appFeatures.svelte';
	import { installGlobalErrorHandling } from '$lib/bootstrap/globalErrorHandling';
	import Toast from '$lib/components/toasts/Toast.svelte';
	import { fade } from 'svelte/transition';
	import { useSessionAccess } from '$lib/features/auth/useSessionAccess.svelte';
	import { themeStore } from '$lib/stores/theme.svelte';
	import '../app.css';
	import AppShell from './AppShell.svelte';
	import Login from './login.svelte';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';

	// Eagerly initialize the theme store for the whole app.
	void themeStore;
	void installGlobalErrorHandling();

	interface Props {
		children?: import('svelte').Snippet;
	}

	let { children }: Props = $props();
	const session = useSessionAccess();

	onMount(() => {
		appFeatures.ensureBoot();
	});
</script>

<svelte:head>
	<title>{page.data?.title || m.app_title({ locale: i18n.languageTag })}</title>
</svelte:head>

{#if !session.isAuthenticated}
	<Login />
{:else}
	<AppShell {session}>
		{@render children?.()}
	</AppShell>
{/if}

<Modals>
	{#snippet backdrop({ close })}
		<div
			class="fixed inset-0 z-40 max-h-full max-w-full bg-black/20 backdrop-blur-sm"
			transition:fade|global
			onclick={() => close()}
			onkeydown={(e) => {
				if (e.key === 'Enter' || e.key === ' ') close();
			}}
			role="button"
			tabindex="0"
			aria-label={m.aria_close_modal({ locale: i18n.languageTag })}
		></div>
	{/snippet}
</Modals>

<Toast />
