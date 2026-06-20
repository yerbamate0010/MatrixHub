<script lang="ts">
	import { page } from '$app/state';
	import GridDots from '~icons/tabler/grid-dots';
	import Bell from '~icons/tabler/bell';
	import Wand from '~icons/tabler/wand';
	import * as m from '$lib/paraglide/messages.js';

	const sections = $derived([
		{
			href: '/system/matrix',
			label: m.menu_matrix_led(),
			icon: GridDots
		},
		{
			href: '/system/matrix/alarms',
			label: m.menu_matrix_alarms(),
			icon: Bell
		},
		{
			href: '/system/matrix/effects',
			label: m.menu_matrix_effects(),
			icon: Wand
		}
	]);
</script>

<nav class="tabs tabs-box mb-3 w-full sm:mb-4 sm:w-auto" aria-label={m.matrix_nav_label()}>
	{#each sections as section}
		<a
			href={section.href}
			class="tab gap-2 text-sm"
			class:tab-active={page.url.pathname === section.href}
			aria-current={page.url.pathname === section.href ? 'page' : undefined}
		>
			<section.icon class="h-4 w-4" />
			<span>{section.label}</span>
		</a>
	{/each}
</nav>
