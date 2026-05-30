<!-- @runes -->
<script lang="ts">
	import { browser } from '$app/environment';
	import { semantic } from '$lib/styles/design-system';
	import { createHtmlId } from '$lib/utils/dom/htmlId';

	interface Props {
		label?: string;
		id?: string;
		value?: string;
		placeholder?: string;
		rows?: number;
		required?: boolean;
		disabled?: boolean;
		readonly?: boolean;
		error?: string;
		help?: string;
		minlength?: number;
		maxlength?: number;
		ariaLabel?: string;
		name?: string;
		class?: string;
	}

	let {
		label,
		id = '',
		value = $bindable(''),
		placeholder = '',
		rows = 4,
		required = false,
		disabled = false,
		readonly = false,
		error = '',
		help = '',
		minlength,
		maxlength,
		ariaLabel,
		name,
		class: className = ''
	}: Props = $props();

	let fallbackId = $state('');

	$effect(() => {
		const hasProvidedId = !!id?.trim();
		if (!hasProvidedId && browser && !fallbackId) {
			fallbackId = createHtmlId('textarea');
		}
		if (hasProvidedId && fallbackId) {
			fallbackId = '';
		}
	});

	const resolvedId = $derived(id?.trim() ? id : fallbackId);
	const resolvedName = $derived(name?.trim() ? name : resolvedId);

	const hasError = $derived(!!error);
	const textareaClasses = $derived(
		`textarea textarea-bordered w-full ${hasError ? 'border-error border-2' : ''} ${className}`
	);

	const handleInput = (e: Event) => {
		const target = e.target as HTMLTextAreaElement;
		value = target?.value ?? '';
	};
</script>

<div class={semantic.form.field}>
	{#if label}
		<label class={semantic.form.label} for={resolvedId || undefined}>{label}</label>
	{/if}

	<textarea
		id={resolvedId || undefined}
		name={resolvedName || undefined}
		{placeholder}
		{rows}
		{required}
		{disabled}
		{readonly}
		{minlength}
		{maxlength}
		aria-label={ariaLabel || label}
		class={textareaClasses}
		{value}
		oninput={handleInput}
	></textarea>

	{#if error}
		<span class={semantic.form.error}>{error}</span>
	{:else if help}
		<span class={semantic.form.help}>{help}</span>
	{/if}
</div>
