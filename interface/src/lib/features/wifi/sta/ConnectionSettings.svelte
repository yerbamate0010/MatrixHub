<script lang="ts">
	import FeatureHelpModal from '$lib/components/help/FeatureHelpModal.svelte';
	import HelpTriggerButton from '$lib/components/help/HelpTriggerButton.svelte';
	import Save from '~icons/tabler/device-floppy';
	import Settings from '~icons/tabler/settings';
	import BaseCard from '$lib/components/layout/BaseCard.svelte';
	import { FormButton, FormInput, FormSelect } from '$lib/components/shared/forms';
	import ContentBox from '$lib/components/layout/ContentBox.svelte';
	import { WifiHostnameSchema } from '$lib/features/wifi/wifiValidation';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';
	import { useZodForm } from '$lib/utils/validation/zodForm.svelte';
	import { z } from 'zod';

	interface Props {
		hostname: string;
		connectionMode: number;
		isDirty?: boolean;
		saveBlocked?: boolean;
		onApply?: () => void;
		onHostnameChange: (_value: string) => void;
		onConnectionModeChange: (_value: number) => void;
	}

	let {
		hostname,
		connectionMode,
		isDirty = false,
		saveBlocked = false,
		onApply,
		onHostnameChange,
		onConnectionModeChange
	}: Props = $props();

	// Single-field schema for just the connection settings part
	const connectionSchema = z.object({
		hostname: WifiHostnameSchema,
		connectionMode: z.number().min(0).max(1)
	});

	const form = useZodForm({
		schema: connectionSchema,
		// use unwrap to avoid warning about capturing initial values of props
		initialValues: { hostname: '', connectionMode: 0 },
		onSubmit: (values) => {
			onHostnameChange(values.hostname);
			onConnectionModeChange(values.connectionMode);
		}
	});

	// Initial sync from props (one-time)
	let initialized = $state(false);
	$effect(() => {
		if (!initialized && (hostname || connectionMode !== undefined)) {
			form.handleInput('hostname', hostname, false);
			form.handleInput('connectionMode', connectionMode, false);
			initialized = true;
		}
	});

	// Sync from props if they change after initialization
	$effect(() => {
		if (initialized) {
			if (hostname !== form.values.hostname) {
				form.handleInput('hostname', hostname, false);
			}
			if (connectionMode !== form.values.connectionMode) {
				form.handleInput('connectionMode', connectionMode, false);
			}
		}
	});

	// Sync form values back to parent on change/blur
	function updateParent() {
		onHostnameChange(form.values.hostname);
		onConnectionModeChange(form.values.connectionMode);
	}

	const connectionModes = [
		{ id: 0, text: m.wifi_connection_mode_disabled({ locale: i18n.languageTag }) },
		{ id: 1, text: m.wifi_connection_mode_auto({ locale: i18n.languageTag }) }
	];
	let helpOpen = $state(false);
	const locale = $derived(i18n.languageTag);
	const helpSections = $derived([
		{
			title: m.help_modal_how_title({ locale }),
			body: m.wifi_sta_help_how_body({ locale })
		},
		{
			title: m.help_modal_setup_title({ locale }),
			body: m.wifi_sta_help_setup_body({ locale })
		},
		{
			title: m.help_modal_watch_title({ locale }),
			body: m.wifi_sta_help_watch_body({ locale })
		}
	]);
	const helpLinks = $derived([
		{ href: '/wifi/ap', label: m.menu_wifi_ap({ locale }) },
		{ href: '/system/status', label: m.menu_status({ locale }) },
		{ href: '/system/help', label: m.menu_help({ locale }) }
	]);
</script>

<BaseCard
	title={m.wifi_connection_title({ locale: i18n.languageTag })}
	icon={Settings}
	class="h-full"
>
	{#snippet actions()}
		<HelpTriggerButton
			label={m.wifi_sta_help_title({ locale })}
			onclick={() => (helpOpen = true)}
		/>
	{/snippet}

	<div class="flex flex-col gap-1">
		<ContentBox>
			<FormInput
				id="hostname"
				label={m.wifi_hostname_label({ locale: i18n.languageTag })}
				help={m.wifi_hostname_help({ locale: i18n.languageTag })}
				value={form.values.hostname}
				placeholder="matrixhub"
				oninput={(e) => {
					const val = (e.target as HTMLInputElement).value;
					form.handleInput('hostname', val);
					updateParent();
				}}
				onblur={() => form.handleBlur('hostname')}
				error={form.errors.hostname}
				class="w-full"
				minlength={3}
				maxlength={32}
				required
			/>
		</ContentBox>

		<ContentBox>
			<FormSelect
				id="apmode"
				label={m.wifi_connection_mode_label({ locale: i18n.languageTag })}
				value={form.values.connectionMode}
				onchange={(e) => {
					const val = Number((e.target as HTMLSelectElement).value);
					form.handleInput('connectionMode', val);
					updateParent();
				}}
				options={connectionModes.map((m) => ({ value: m.id, label: m.text }))}
			/>
		</ContentBox>

		{#if onApply}
			<div class="mt-4 flex justify-end md:hidden">
				<FormButton
					label={m.action_save({ locale: i18n.languageTag })}
					icon={Save}
					type="button"
					disabled={!isDirty || saveBlocked}
					onclick={onApply}
				/>
			</div>
		{/if}
	</div>

	<FeatureHelpModal
		isOpen={helpOpen}
		onClose={() => (helpOpen = false)}
		title={m.wifi_sta_help_title({ locale })}
		intro={m.wifi_sta_help_intro({ locale })}
		sections={helpSections}
		links={helpLinks}
	/>
</BaseCard>
