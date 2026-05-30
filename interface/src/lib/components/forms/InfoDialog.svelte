<script lang="ts">
	import Check from '~icons/tabler/check';
	import Modal from '$lib/components/Modal.svelte';
	import FormButton from '$lib/components/shared/forms/FormButton.svelte';
	import { closeModal } from '$lib/utils/ui/modal';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';

	import type { Component } from 'svelte';
	// provided by <Modals />

	interface Props {
		isOpen: boolean;
		title: string;
		message?: string;
		// Use only for trusted markup. Prefer `message` for plain text.
		messageHtml?: string;
		onDismiss: () => void;
		dismiss?: { label: string; icon: Component } | (() => void);
	}

	let {
		isOpen = false,
		title = '',
		message = '',
		messageHtml,
		onDismiss = () => {},
		dismiss
	}: Props = $props();

	const fallbackDismiss = $derived({
		label: m.action_dismiss({ locale: i18n.languageTag }),
		icon: Check
	});

	// Compatibility:
	// - In this app, `dismiss` is commonly used as button config: { label, icon }.
	// - Some modal systems provide a `dismiss()` function prop to close the modal.
	const modalDismissFn = $derived(
		typeof dismiss === 'function' ? (dismiss as () => void) : undefined
	);
	const dismissButton = $derived(
		dismiss && typeof dismiss === 'object'
			? (dismiss as { label: string; icon: Component })
			: fallbackDismiss
	);

	function handleDismiss() {
		try {
			onDismiss?.();
		} finally {
			if (modalDismissFn) {
				modalDismissFn();
			} else {
				closeModal();
			}
		}
	}
</script>

<Modal
	{isOpen}
	onClose={handleDismiss}
	{title}
	widthClass="min-w-fit max-w-md"
	paddingClass="p-4"
	bodyClass="min-h-0 flex-1 overflow-y-auto"
	actionsClass="flex justify-end gap-2"
>
	{#if messageHtml}
		<p class="text-base-content mb-1 text-start">{@html messageHtml}</p>
	{:else}
		<p class="text-base-content mb-1 text-start whitespace-pre-line">{message}</p>
	{/if}

	{#snippet actions()}
		<FormButton
			label={dismissButton.label}
			icon={dismissButton.icon}
			class="btn-warning text-warning-content"
			onclick={handleDismiss}
		/>
	{/snippet}
</Modal>
