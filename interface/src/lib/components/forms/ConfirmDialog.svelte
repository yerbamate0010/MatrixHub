<script lang="ts">
	import { type ModalProps } from 'svelte-modals';
	import Modal from '$lib/components/Modal.svelte';
	import FormButton from '$lib/components/shared/forms/FormButton.svelte';
	import { closeModal } from '$lib/utils/ui/modal';
	import Cancel from '~icons/tabler/x';
	import Check from '~icons/tabler/check';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';

	interface Props extends ModalProps {
		title: string;
		message?: string;
		// Use only for trusted markup. Prefer `message` for plain text.
		messageHtml?: string;
		onConfirm: () => void;
		onCancel?: () => void;
		labels?: {
			cancel: { label: string; icon: typeof Cancel };
			confirm: { label: string; icon: typeof Cancel };
		};
	}

	let { isOpen, title, message = '', messageHtml, onConfirm, onCancel, labels }: Props = $props();

	const fallbackLabels = $derived({
		cancel: { label: m.action_cancel({ locale: i18n.languageTag }), icon: Cancel },
		confirm: { label: m.action_confirm({ locale: i18n.languageTag }), icon: Check }
	});

	const finalLabels = $derived({
		cancel: { ...fallbackLabels.cancel, ...labels?.cancel },
		confirm: { ...fallbackLabels.confirm, ...labels?.confirm }
	});

	function handleConfirm() {
		onConfirm?.();
		closeModal();
	}

	function handleCancel() {
		onCancel?.();
		closeModal();
	}
</script>

<Modal
	{isOpen}
	onClose={handleCancel}
	{title}
	widthClass="w-full max-w-xs sm:max-w-sm md:max-w-md"
	paddingClass="p-4"
	bodyClass="min-h-0 flex-1 overflow-y-auto"
	actionsClass="flex justify-end gap-2"
>
	{#if messageHtml}
		<p class="text-base-content mb-1 text-start break-words">
			{@html messageHtml}
		</p>
	{:else}
		<p class="text-base-content mb-1 text-start break-words whitespace-pre-line">{message}</p>
	{/if}

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
