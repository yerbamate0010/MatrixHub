<script lang="ts">
	import { untrack } from 'svelte';
	import { type ModalProps } from 'svelte-modals';
	import Modal from '$lib/components/Modal.svelte';
	import { FormButton, FormInput } from '$lib/components/shared/forms';
	import { closeModal } from '$lib/utils/ui/modal';
	import Cancel from '~icons/tabler/x';
	import Check from '~icons/tabler/check';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';

	interface Props extends ModalProps {
		title: string;
		message?: string;
		defaultValue?: string;
		placeholder?: string;
		maxlength?: number;
		asciiOnly?: boolean;
		onConfirm: (value: string) => void;
		onCancel?: () => void;
		labels?: {
			cancel: { label: string; icon: typeof Cancel };
			confirm: { label: string; icon: typeof Cancel };
		};
	}

	let {
		isOpen,
		title,
		message = '',
		defaultValue = '',
		placeholder = '',
		maxlength,
		asciiOnly = false,
		onConfirm,
		onCancel,
		labels
	}: Props = $props();

	let value = $state(untrack(() => defaultValue));

	const fallbackLabels = $derived({
		cancel: { label: m.action_cancel({ locale: i18n.languageTag }), icon: Cancel },
		confirm: { label: m.action_save({ locale: i18n.languageTag }), icon: Check }
	});

	const finalLabels = $derived({
		cancel: { ...fallbackLabels.cancel, ...labels?.cancel },
		confirm: { ...fallbackLabels.confirm, ...labels?.confirm }
	});

	function handleConfirm() {
		closeModal();
		onConfirm?.(value);
	}

	function handleCancel() {
		onCancel?.();
		closeModal();
	}

	function handleKeydown(e: KeyboardEvent) {
		if (e.key === 'Enter') {
			handleConfirm();
		}
	}

	function handleInput(e: Event) {
		const target = e.target as HTMLInputElement;
		let next = target.value;
		if (asciiOnly) {
			next = next.replace(/[^\x20-\x7E]/g, '');
		}
		if (typeof maxlength === 'number') {
			next = next.slice(0, maxlength);
		}
		value = next;
	}
</script>

<Modal
	{isOpen}
	onClose={handleCancel}
	{title}
	widthClass="w-full max-w-xs sm:max-w-sm md:max-w-md"
	paddingClass="p-4"
	bodyClass="min-h-0 flex-1 overflow-y-auto p-1"
	actionsClass="flex justify-end gap-2"
>
	{#if message}
		<p class="text-base-content mb-3 text-start break-words whitespace-pre-line">{message}</p>
	{/if}

	<!-- svelte-ignore a11y_autofocus -->
	<FormInput
		type="text"
		{placeholder}
		{maxlength}
		bind:value
		onkeydown={handleKeydown}
		oninput={handleInput}
		autofocus
	/>

	{#snippet actions()}
		<FormButton
			label={finalLabels.cancel.label}
			icon={finalLabels.cancel.icon}
			class="btn-neutral"
			onclick={handleCancel}
		/>
		<FormButton
			label={finalLabels.confirm.label}
			icon={finalLabels.confirm.icon}
			class="btn-primary"
			onclick={handleConfirm}
		/>
	{/snippet}
</Modal>
