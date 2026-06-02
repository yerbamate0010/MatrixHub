<script lang="ts">
	import BaseCard from '$lib/components/layout/BaseCard.svelte';
	import ContentBox from '$lib/components/layout/ContentBox.svelte';
	import { FormButton, FormRange, FormToggle } from '$lib/components/shared/forms';
	import { Spinner } from '$lib/components/common';
	import IconPalette from '~icons/tabler/palette';
	import IconMoodSmile from '~icons/tabler/mood-smile';
	import IconMarquee from '~icons/tabler/marquee';
	import IconGridDots from '~icons/tabler/grid-dots';
	import IconEditorModal from '$lib/components/matrix/IconEditorModal.svelte';
	import { type useMatrixSettings } from './useMatrixSettings.svelte';
	import {
		fromMatrixHexColor,
		getMatrixCustomIcons,
		MATRIX_MENU_BUTTON_LOCKED_ENABLED,
		toMatrixHexColor
	} from './matrixModel';

	import * as m from '$lib/paraglide/messages.js';

	let {
		store,
		canManage = true
	}: {
		store: ReturnType<typeof useMatrixSettings>;
		canManage?: boolean;
	} = $props();

	// Helpers for radio selection
	const modes = [
		{ value: 0, label: m.matrix_mode_solid(), desc: m.matrix_mode_solid_desc(), icon: IconPalette },
		{ value: 1, label: m.matrix_mode_icon(), desc: m.matrix_mode_icon_desc(), icon: IconMoodSmile },
		{
			value: 2,
			label: m.matrix_mode_scroll(),
			desc: m.matrix_mode_scroll_desc(),
			icon: IconMarquee
		}
	];

	let showCustomIcons = $state(false);
	const alarmUsesIcons = $derived(store.settings.alarm_mode === 1);

	// Derived hex value for local input binding
	let menuColorHex = $state('#FFFFFF');

	// Watch for external updates to sync input
	$effect(() => {
		if (!store.loading) {
			menuColorHex = toMatrixHexColor(store.settings.menu_text_color);
		}
	});

	function handleMenuColorChange(e: Event) {
		if (!canManage) return;
		const target = e.target as HTMLInputElement;
		menuColorHex = target.value;
		store.settings.menu_text_color = fromMatrixHexColor(menuColorHex);
	}

	async function handleIconsSave(newIcons: number[][]) {
		if (!canManage) return false;
		store.settings = { ...store.settings, custom_icons: newIcons };
		if (!store.hasChanges) return true;
		return await store.saveSettingsNow();
	}
</script>

<BaseCard title={m.matrix_title()} icon={IconGridDots}>
	<div class="flex w-full flex-col gap-1">
		{#if store.loading}
			<div class="flex justify-center items-center py-8">
				<Spinner />
			</div>
		{:else if store.error}
			<div class="text-error text-sm p-2 bg-error/10 rounded">
				{store.error}
			</div>
		{:else}
			<ContentBox title={m.matrix_section_alarm()} class="flex flex-col gap-4">
				<div class="flex flex-col gap-3">
					<span class="font-bold text-sm">{m.matrix_mode_label()}</span>
					<div class="grid grid-cols-1 md:grid-cols-3 gap-3">
						{#each modes as mode}
							<button
								class="flex items-center space-x-3 border rounded-md p-2 cursor-pointer hover:bg-base-200 transition-colors text-left
                                    {store.settings.alarm_mode === mode.value
									? 'bg-base-200 border-primary ring-1 ring-primary'
									: 'border-base-300'}"
								onclick={() => (store.settings.alarm_mode = mode.value)}
								disabled={!canManage}
								type="button"
								aria-pressed={store.settings.alarm_mode === mode.value}
							>
								<div
									class="p-2 rounded-full {store.settings.alarm_mode === mode.value
										? 'bg-primary text-primary-content'
										: 'bg-base-300 text-base-content'}"
								>
									<mode.icon class="w-5 h-5" />
								</div>
								<div class="flex flex-col">
									<span class="font-medium text-sm">{mode.label}</span>
									<span class="text-[10px] text-base-content/70">{mode.desc}</span>
								</div>
							</button>
						{/each}
					</div>
				</div>

				<div
					class="rounded-box border p-4 flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between {alarmUsesIcons
						? 'border-primary/50 bg-primary/5'
						: 'border-base-300 bg-base-200/40'}"
				>
					<div class="min-w-0">
						<div class="flex flex-wrap items-center gap-2">
							<span class="font-bold text-sm">{m.matrix_custom_icons_title()}</span>
							{#if alarmUsesIcons}
								<span class="badge badge-primary badge-sm">{m.matrix_mode_icon()}</span>
							{/if}
						</div>
						<p class="text-xs text-base-content/70 mt-1">{m.matrix_custom_icons_desc()}</p>
					</div>
					<FormButton
						label={m.matrix_edit_icons_btn()}
						class="btn-sm btn-outline w-full sm:w-auto"
						disabled={!canManage}
						onclick={() => (showCustomIcons = true)}
					/>
				</div>

				<IconEditorModal
					isOpen={showCustomIcons}
					onClose={() => (showCustomIcons = false)}
					customIcons={getMatrixCustomIcons(store.settings.custom_icons)}
					onSave={handleIconsSave}
				/>
			</ContentBox>

			<ContentBox title={m.matrix_section_menu()} class="flex flex-col gap-4">
				<div class="flex items-center justify-between gap-3">
					<div>
						<span class="font-bold text-sm">{m.matrix_menu_enabled()}</span>
						<p class="text-xs text-base-content/70">{m.matrix_menu_enabled_desc()}</p>
					</div>
					<FormToggle
						label=""
						checked={MATRIX_MENU_BUTTON_LOCKED_ENABLED}
						disabled={true}
						ariaLabel={m.matrix_menu_enabled()}
						plain={true}
					/>
				</div>

				<div class="flex items-center justify-between gap-3">
					<div>
						<span class="font-bold text-sm">{m.matrix_menu_text_color()}</span>
						<p class="text-xs text-base-content/70">{m.matrix_menu_text_color_desc()}</p>
					</div>
					<input
						type="color"
						class="input input-bordered p-0 w-12 h-8 cursor-pointer shrink-0"
						value={menuColorHex}
						disabled={!canManage}
						aria-label={m.matrix_menu_text_color()}
						oninput={handleMenuColorChange}
					/>
				</div>

				<FormRange
					label={m.matrix_scroll_speed()}
					min={20}
					max={120}
					step={5}
					suffix="ms"
					disabled={!canManage}
					bind:value={store.settings.menu_scroll_speed}
				/>
			</ContentBox>

			<ContentBox title={m.matrix_section_display()} class="flex flex-col gap-4">
				<FormRange
					label={m.matrix_brightness()}
					min={2}
					max={255}
					step={1}
					disabled={!canManage}
					bind:value={store.settings.brightness}
				/>

				<div class="flex flex-col gap-3">
					<div class="flex items-center justify-between gap-3">
						<span class="font-bold text-sm">{m.matrix_rotation()}</span>
						<div class="flex items-center gap-2">
							<span class="text-xs opacity-70">{m.matrix_auto()}</span>
							<FormToggle
								label=""
								bind:checked={store.settings.auto_rotate}
								disabled={!canManage}
								ariaLabel={m.matrix_auto()}
								plain={true}
							/>
						</div>
					</div>

					<div class="grid grid-cols-2 gap-2 sm:grid-cols-4">
						{#each [0, 1, 2, 3] as rot}
							<FormButton
								label={rot === 0 ? '0°' : rot === 1 ? '90°' : rot === 2 ? '180°' : '270°'}
								class="btn-sm flex-1 {store.settings.rotation === rot
									? 'btn-primary'
									: 'btn-outline border-base-300'}"
								disabled={!canManage || store.settings.auto_rotate}
								onclick={() => (store.settings.rotation = rot)}
							/>
						{/each}
					</div>
				</div>
			</ContentBox>
		{/if}
	</div>
</BaseCard>
