<script lang="ts">
	import BaseCard from '$lib/components/layout/BaseCard.svelte';
	import ContentBox from '$lib/components/layout/ContentBox.svelte';
	import { FormButton, FormRange, FormSelect, FormToggle } from '$lib/components/shared/forms';
	import { Spinner } from '$lib/components/common';
	import DeviceFloppy from '~icons/tabler/device-floppy';
	import IconWand from '~icons/tabler/wand';
	import { type useMatrixSettings } from './useMatrixSettings.svelte';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';
	import {
		type MatrixEffectSpeedScale,
		fromMatrixHexColor,
		MATRIX_EFFECT_IDS,
		MATRIX_EFFECT_SPEED_SCALE_CONFIG,
		MATRIX_EFFECT_SPEED_SCALES,
		fromMatrixEffectSpeedScaleValue,
		getPreferredMatrixEffectSpeedScale,
		normalizeMatrixEffectSpeedForScale,
		toMatrixHexColor
	} from './matrixModel';

	type MatrixEffectCategoryId = 'recommended' | 'calm' | 'dynamic' | 'seasonal' | 'all';

	type MatrixEffectCategory = {
		value: MatrixEffectCategoryId;
		label: string;
		effectIds: number[];
	};

	type MatrixColorPreset = {
		id: string;
		label: string;
		colors: [number, number, number];
	};

	let {
		store,
		canManage = true
	}: {
		store: ReturnType<typeof useMatrixSettings>;
		canManage?: boolean;
	} = $props();

	// Derived hex value for local input binding
	let hexColor = $state('#00FF00');
	let hexColor2 = $state('#000000');
	let hexColor3 = $state('#0000FF');
	let speedScale = $state<MatrixEffectSpeedScale>('ms');
	let speedSliderValue = $state(1000);
	let speedScaleInitialized = $state(false);
	let effectCategory = $state<MatrixEffectCategoryId>('recommended');
	let effectCategoryInitialized = $state(false);
	let pendingEffectAutosave = $state(false);

	const effectCategories = $derived.by<MatrixEffectCategory[]>(() => [
		{
			value: 'recommended',
			label: m.matrix_effect_category_recommended({ locale: i18n.languageTag }),
			effectIds: [2, 11, 44, 48, 65]
		},
		{
			value: 'calm',
			label: m.matrix_effect_category_calm({ locale: i18n.languageTag }),
			effectIds: [0, 1, 2, 15, 18, 40, 64]
		},
		{
			value: 'dynamic',
			label: m.matrix_effect_category_dynamic({ locale: i18n.languageTag }),
			effectIds: [3, 11, 12, 16, 17, 31, 33, 44, 67, 69]
		},
		{
			value: 'seasonal',
			label: m.matrix_effect_category_seasonal({ locale: i18n.languageTag }),
			effectIds: [45, 47, 48, 49, 50, 52, 56, 63]
		},
		{
			value: 'all',
			label: m.matrix_effect_category_all({ locale: i18n.languageTag }),
			effectIds: MATRIX_EFFECT_IDS
		}
	]);

	const colorPresets = $derived.by<MatrixColorPreset[]>(() => [
		{
			id: 'alert',
			label: m.matrix_effect_palette_alert({ locale: i18n.languageTag }),
			colors: [0x7f0000, 0xff5a36, 0xffd166]
		},
		{
			id: 'forest',
			label: m.matrix_effect_palette_forest({ locale: i18n.languageTag }),
			colors: [0x0b3d20, 0x2e7d32, 0xa5d6a7]
		},
		{
			id: 'ocean',
			label: m.matrix_effect_palette_ocean({ locale: i18n.languageTag }),
			colors: [0x003049, 0x0077b6, 0x90e0ef]
		},
		{
			id: 'sunset',
			label: m.matrix_effect_palette_sunset({ locale: i18n.languageTag }),
			colors: [0x5f0f40, 0xfb8b24, 0xffbe0b]
		},
		{
			id: 'neon',
			label: m.matrix_effect_palette_neon({ locale: i18n.languageTag }),
			colors: [0xff006e, 0x8338ec, 0x3a86ff]
		},
		{
			id: 'aurora',
			label: m.matrix_effect_palette_aurora({ locale: i18n.languageTag }),
			colors: [0x2ec4b6, 0x7b2cbf, 0xc2f970]
		}
	]);

	function getCategoryById(categoryId: MatrixEffectCategoryId): MatrixEffectCategory | undefined {
		return effectCategories.find((category) => category.value === categoryId);
	}

	function categoryContainsEffect(categoryId: MatrixEffectCategoryId, effectId: number): boolean {
		return getCategoryById(categoryId)?.effectIds.includes(effectId) ?? false;
	}

	function getPreferredEffectCategory(effectId: number): MatrixEffectCategoryId {
		for (const category of effectCategories) {
			if (category.value === 'all') continue;
			if (category.effectIds.includes(effectId)) {
				return category.value;
			}
		}

		return 'all';
	}

	// Watch for external updates to sync input
	$effect(() => {
		if (!store.loading) {
			hexColor = toMatrixHexColor(store.settings.effect_color);
			hexColor2 = toMatrixHexColor(store.settings.effect_color_2);
			hexColor3 = toMatrixHexColor(store.settings.effect_color_3);

			if (!speedScaleInitialized) {
				speedScale = getPreferredMatrixEffectSpeedScale(store.settings.effect_speed);
				speedScaleInitialized = true;
			}
			speedSliderValue =
				normalizeMatrixEffectSpeedForScale(store.settings.effect_speed, speedScale) /
				MATRIX_EFFECT_SPEED_SCALE_CONFIG[speedScale].unitMs;

			if (!effectCategoryInitialized) {
				effectCategory = getPreferredEffectCategory(store.settings.effect_mode);
				effectCategoryInitialized = true;
			}
		}
	});

	$effect(() => {
		if (store.loading) {
			speedScaleInitialized = false;
			effectCategoryInitialized = false;
		}
	});

	$effect(() => {
		if (!store.saving && pendingEffectAutosave) {
			pendingEffectAutosave = false;
			void saveSelectedEffectSilently();
		}
	});

	// Handle color change
	function handleColorChange(e: Event) {
		if (!canManage) return;
		const target = e.target as HTMLInputElement;
		hexColor = target.value;
		store.settings.effect_color = fromMatrixHexColor(hexColor);
	}

	function handleColorChange2(e: Event) {
		if (!canManage) return;
		const target = e.target as HTMLInputElement;
		hexColor2 = target.value;
		store.settings.effect_color_2 = fromMatrixHexColor(hexColor2);
	}

	function handleColorChange3(e: Event) {
		if (!canManage) return;
		const target = e.target as HTMLInputElement;
		hexColor3 = target.value;
		store.settings.effect_color_3 = fromMatrixHexColor(hexColor3);
	}

	function handleEffectSpeedChange(e: Event) {
		if (!canManage) return;
		const target = e.target as HTMLInputElement;
		speedSliderValue = Number(target.value);
		store.settings.effect_speed = fromMatrixEffectSpeedScaleValue(speedSliderValue, speedScale);
	}

	function selectSpeedScale(nextScale: MatrixEffectSpeedScale) {
		if (speedScale === nextScale) return;
		speedScale = nextScale;
		speedScaleInitialized = true;

		const normalizedSpeed = normalizeMatrixEffectSpeedForScale(
			store.settings.effect_speed,
			nextScale
		);
		speedSliderValue = normalizedSpeed / MATRIX_EFFECT_SPEED_SCALE_CONFIG[nextScale].unitMs;

		if (canManage) {
			store.settings.effect_speed = normalizedSpeed;
		}
	}

	async function saveSelectedEffectSilently() {
		if (!canManage) return;
		if (store.saving) {
			pendingEffectAutosave = true;
			return;
		}

		const saved = await store.saveSettingsSilentlyNow();
		if (!saved && store.hasChanges) {
			pendingEffectAutosave = true;
		}
	}

	async function applySelectedEffect(effectId: number) {
		if (!canManage || store.settings.effect_mode === effectId) {
			return;
		}

		store.updateSetting('effect_mode', effectId);
		await saveSelectedEffectSilently();
	}

	function handleEffectCategoryChange(e: Event) {
		const nextCategory = (e.currentTarget as HTMLSelectElement).value as MatrixEffectCategoryId;
		effectCategory = nextCategory;
		effectCategoryInitialized = true;

		if (categoryContainsEffect(nextCategory, store.settings.effect_mode)) {
			return;
		}

		const nextEffect = getCategoryById(nextCategory)?.effectIds[0];
		if (nextEffect !== undefined && canManage) {
			void applySelectedEffect(nextEffect);
		}
	}

	function applyColorPreset(preset: MatrixColorPreset) {
		if (!canManage) return;

		store.settings.effect_color = preset.colors[0];
		store.settings.effect_color_2 = preset.colors[1];
		store.settings.effect_color_3 = preset.colors[2];

		hexColor = toMatrixHexColor(preset.colors[0]);
		hexColor2 = toMatrixHexColor(preset.colors[1]);
		hexColor3 = toMatrixHexColor(preset.colors[2]);
	}

	function getEffectName(id: number): string {
		const mapping: Record<number, (args: { locale: string }) => string> = {
			0: m.matrix_eff_static,
			1: m.matrix_eff_blink,
			2: m.matrix_eff_breath,
			3: m.matrix_eff_color_wipe,
			4: m.matrix_eff_color_wipe_inv,
			5: m.matrix_eff_color_wipe_rev,
			6: m.matrix_eff_color_wipe_rev_inv,
			7: m.matrix_eff_color_wipe_random,
			8: m.matrix_eff_random_color,
			9: m.matrix_eff_single_dynamic,
			10: m.matrix_eff_multi_dynamic,
			11: m.matrix_eff_rainbow,
			12: m.matrix_eff_rainbow_cycle,
			13: m.matrix_eff_scan,
			14: m.matrix_eff_dual_scan,
			15: m.matrix_eff_fade,
			16: m.matrix_eff_theater_chase,
			17: m.matrix_eff_theater_chase_rainbow,
			18: m.matrix_eff_running_lights,
			19: m.matrix_eff_twinkle,
			20: m.matrix_eff_twinkle_random,
			21: m.matrix_eff_twinkle_fade,
			22: m.matrix_eff_twinkle_fade_random,
			23: m.matrix_eff_sparkle,
			24: m.matrix_eff_flash_sparkle,
			25: m.matrix_eff_hyper_sparkle,
			26: m.matrix_eff_strobe,
			27: m.matrix_eff_strobe_rainbow,
			28: m.matrix_eff_multi_strobe,
			29: m.matrix_eff_blink_rainbow,
			30: m.matrix_eff_chase_white,
			31: m.matrix_eff_chase_color,
			32: m.matrix_eff_chase_random,
			33: m.matrix_eff_chase_rainbow,
			34: m.matrix_eff_chase_flash,
			35: m.matrix_eff_chase_flash_random,
			36: m.matrix_eff_chase_rainbow_white,
			37: m.matrix_eff_chase_blackout,
			38: m.matrix_eff_chase_blackout_rainbow,
			39: m.matrix_eff_color_sweep_random,
			40: m.matrix_eff_running_color,
			41: m.matrix_eff_running_red_blue,
			42: m.matrix_eff_running_random,
			43: m.matrix_eff_larson_scanner,
			44: m.matrix_eff_comet,
			45: m.matrix_eff_fireworks,
			46: m.matrix_eff_fireworks_random,
			47: m.matrix_eff_merry_christmas,
			48: m.matrix_eff_fire_flicker,
			49: m.matrix_eff_fire_flicker_soft,
			50: m.matrix_eff_fire_flicker_intense,
			51: m.matrix_eff_circus_combustus,
			52: m.matrix_eff_halloween,
			53: m.matrix_eff_bicolor_chase,
			54: m.matrix_eff_tricolor_chase,
			55: m.matrix_eff_twinklefox,
			56: m.matrix_eff_rain,
			57: m.matrix_eff_block_dissolve,
			58: m.matrix_eff_icu,
			59: m.matrix_eff_dual_larson,
			60: m.matrix_eff_running_random2,
			61: m.matrix_eff_filler_up,
			62: m.matrix_eff_rainbow_larson,
			63: m.matrix_eff_rainbow_fireworks,
			64: m.matrix_eff_trifade,
			65: m.matrix_eff_heartbeat,
			66: m.matrix_eff_bits,
			67: m.matrix_eff_multi_comet,
			68: m.matrix_eff_popcorn,
			69: m.matrix_eff_oscillator
		};
		return mapping[id] ? mapping[id]({ locale: i18n.languageTag }) : `Effect ${id}`;
	}

	const effectCategoryOptions = $derived.by(() =>
		effectCategories.map((category) => ({
			value: category.value,
			label: category.label
		}))
	);

	const effectOptions = $derived.by(() =>
		(getCategoryById(effectCategory)?.effectIds ?? MATRIX_EFFECT_IDS).map((effectId) => ({
			value: effectId,
			label: getEffectName(effectId)
		}))
	);

	const speedScaleConfig = $derived.by(() => MATRIX_EFFECT_SPEED_SCALE_CONFIG[speedScale]);
	const activeColorPresetId = $derived.by(() => {
		const activePreset = colorPresets.find(
			(preset) =>
				preset.colors[0] === store.settings.effect_color &&
				preset.colors[1] === store.settings.effect_color_2 &&
				preset.colors[2] === store.settings.effect_color_3
		);

		return activePreset?.id;
	});
</script>

<BaseCard title={m.matrix_effects_title()} icon={IconWand}>
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
			<!-- Master Toggle -->
			<!-- Master Toggle -->
			<ContentBox class="flex items-center justify-between">
				<div>
					<span class="font-bold text-sm">{m.matrix_effects_enable()}</span>
					<p class="text-xs text-base-content/70">{m.matrix_effects_desc()}</p>
				</div>
				<FormToggle
					label=""
					bind:checked={store.settings.effect_enabled}
					disabled={!canManage}
					ariaLabel={m.matrix_effects_enable({ locale: i18n.languageTag })}
					plain={true}
				/>
			</ContentBox>

			<!-- Effect Settings (Greyed out if disabled) -->
			<div
				class="flex flex-col gap-1 transition-opacity duration-200 {store.settings.effect_enabled
					? 'opacity-100'
					: 'opacity-50 pointer-events-none'}"
			>
				<ContentBox>
					<FormSelect
						label={m.matrix_effect_category()}
						value={effectCategory}
						options={effectCategoryOptions}
						disabled={!canManage}
						onchange={handleEffectCategoryChange}
					/>
				</ContentBox>

				<!-- Mode Selection -->
				<!-- Mode Selection -->
				<ContentBox>
					<FormSelect
						label={m.matrix_effect_mode()}
						value={store.settings.effect_mode}
						options={effectOptions}
						help={m.matrix_effect_mode_live_desc({ locale: i18n.languageTag })}
						disabled={!canManage}
						onchange={(e) =>
							void applySelectedEffect(Number((e.target as HTMLSelectElement).value))}
					/>
				</ContentBox>

				<!-- Speed Configuration -->
				<!-- Speed Configuration -->
				<ContentBox>
					<FormRange
						label={m.matrix_effect_speed()}
						min={speedScaleConfig.min}
						max={speedScaleConfig.max}
						step={speedScaleConfig.step}
						suffix={speedScaleConfig.suffix}
						valueClass="w-20"
						disabled={!canManage}
						bind:value={speedSliderValue}
						oninput={handleEffectSpeedChange}
					/>
					<div class="mt-4 grid grid-cols-2 gap-3 sm:grid-cols-4">
						{#each MATRIX_EFFECT_SPEED_SCALES as scale}
							<button
								type="button"
								class="btn btn-sm h-10 w-full font-semibold uppercase tracking-wide {speedScale ===
								scale
									? 'btn-primary'
									: 'btn-outline btn-primary'}"
								disabled={!canManage}
								aria-pressed={speedScale === scale}
								onclick={() => selectSpeedScale(scale)}
							>
								{scale}
							</button>
						{/each}
					</div>
				</ContentBox>

				<!-- Color Selection -->
				<!-- Color Selection -->
				<ContentBox class="flex flex-col gap-3">
					<div>
						<span class="font-bold text-sm">{m.matrix_effect_palettes()}</span>
						<p class="text-xs text-base-content/70">{m.matrix_effect_palettes_desc()}</p>
					</div>
					<div class="grid grid-cols-2 gap-2 2xl:grid-cols-3">
						{#each colorPresets as preset}
							<button
								type="button"
								class="btn btn-sm h-auto min-h-0 justify-start gap-3 px-3 py-2 {activeColorPresetId ===
								preset.id
									? 'btn-primary'
									: 'btn-outline btn-primary'}"
								disabled={!canManage}
								aria-pressed={activeColorPresetId === preset.id}
								onclick={() => applyColorPreset(preset)}
							>
								<span class="flex items-center gap-1">
									{#each preset.colors as color}
										<span
											class="h-3 w-3 rounded-full border border-base-content/20"
											style={`background-color: ${toMatrixHexColor(color)};`}
										></span>
									{/each}
								</span>
								<span>{preset.label}</span>
							</button>
						{/each}
					</div>
				</ContentBox>

				<ContentBox class="flex items-center justify-between">
					<div>
						<span class="font-bold text-sm">{m.matrix_base_color()}</span>
						<p class="text-xs text-base-content/70">{m.matrix_color_desc()}</p>
					</div>
					<div class="flex flex-col gap-1">
						<div class="flex items-center gap-2 justify-end">
							<span class="font-mono text-xs opacity-70">1: {hexColor}</span>
							<input
								type="color"
								class="input input-bordered p-0 w-12 h-8 cursor-pointer"
								value={hexColor}
								disabled={!canManage}
								aria-label={m.matrix_effect_color_primary({ locale: i18n.languageTag })}
								oninput={handleColorChange}
							/>
						</div>
						<div class="flex items-center gap-2 justify-end">
							<span class="font-mono text-xs opacity-70">2: {hexColor2}</span>
							<input
								type="color"
								class="input input-bordered p-0 w-12 h-8 cursor-pointer"
								value={hexColor2}
								disabled={!canManage}
								aria-label={m.matrix_effect_color_secondary({ locale: i18n.languageTag })}
								oninput={handleColorChange2}
							/>
						</div>
						<div class="flex items-center gap-2 justify-end">
							<span class="font-mono text-xs opacity-70">3: {hexColor3}</span>
							<input
								type="color"
								class="input input-bordered p-0 w-12 h-8 cursor-pointer"
								value={hexColor3}
								disabled={!canManage}
								aria-label={m.matrix_effect_color_tertiary({ locale: i18n.languageTag })}
								oninput={handleColorChange3}
							/>
						</div>
					</div>
				</ContentBox>
			</div>
		{/if}
	</div>

	<div class="mt-4 flex justify-end px-1">
		<FormButton
			onclick={store.saveSettings}
			disabled={!canManage || store.saving || !store.hasChanges || store.loading}
			loading={store.saving}
			label={m.action_save()}
			icon={DeviceFloppy}
		/>
	</div>
</BaseCard>
