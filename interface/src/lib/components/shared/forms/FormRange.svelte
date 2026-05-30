<script lang="ts">
	import type { Snippet } from 'svelte';

	// Range lives with the other shared primitives after the form-system cleanup.
	// Keeping sliders here avoids reintroducing a second form component path.
	let {
		value = $bindable(),
		min = 0,
		max = 100,
		step = 1,
		label,
		description,
		disabled = false,
		class: className = '',
		rangeClass = 'range-primary',
		suffix = '',
		valueClass = '',
		id = undefined,
		...rest
	}: {
		value: number;
		min?: number;
		max?: number;
		step?: number;
		label?: string;
		description?: string | Snippet;
		disabled?: boolean;
		class?: string;
		rangeClass?: string;
		suffix?: string;
		valueClass?: string;
		id?: string;
		oninput?: (e: Event & { currentTarget: EventTarget & HTMLInputElement }) => void;
		onchange?: (e: Event & { currentTarget: EventTarget & HTMLInputElement }) => void;
	} = $props();
</script>

<div class="form-control {className}">
	{#if label}
		<label class="label" for={id}>
			<span class="text-sm font-bold">{label}</span>
		</label>
	{/if}

	<div class="flex items-center gap-2">
		<input
			type="range"
			class="range {rangeClass} range-sm flex-1"
			bind:value
			{min}
			{max}
			{step}
			{disabled}
			{id}
			{...rest}
		/>
		{#if suffix}
			<span
				class="text-xs text-right font-mono tabular-nums whitespace-nowrap {valueClass || 'w-20'}"
				>{value} {suffix}</span
			>
		{:else}
			<span
				class="text-xs text-right font-mono tabular-nums whitespace-nowrap {valueClass || 'w-12'}"
				>{value}</span
			>
		{/if}
	</div>

	{#if description}
		<div class="label">
			{#if typeof description === 'string'}
				<span class="label-text-alt break-words whitespace-normal block">{description}</span>
			{:else}
				{@render description()}
			{/if}
		</div>
	{/if}
</div>
