<script lang="ts">
	import type { Snippet } from 'svelte';

	let {
		children,
		title = undefined,
		class: className = ''
	}: {
		children: Snippet;
		title?: string;
		class?: string;
	} = $props();
</script>

<div class="rounded-box bg-base-100 p-4 {className}">
	{#if title}
		<h4 class="font-bold text-sm mb-2 opacity-70 uppercase tracking-wide">
			{title}
		</h4>
	{/if}
	{@render children()}
</div>
