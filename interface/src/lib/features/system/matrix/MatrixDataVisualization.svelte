<script lang="ts">
	import SettingsCard from '$lib/components/layout/SettingsCard.svelte';
	import ContentBox from '$lib/components/layout/ContentBox.svelte';
	import {
		FormButton,
		FormColorInput,
		FormInput,
		FormRange,
		FormSelect,
		FormToggle
	} from '$lib/components/shared/forms';
	import { Spinner } from '$lib/components/common';
	import { BleApiService } from '$lib/services/api/connectivity/BleApiService';
	import { MatrixApiService } from '$lib/services/api/core/MatrixApiService';
	import { useSessionAccess } from '$lib/features/auth/useSessionAccess.svelte';
	import { i18n } from '$lib/i18n.svelte';
	import type { BleStatus } from '$lib/types/connectivity/ble';
	import ChartBar from '~icons/tabler/chart-bar';
	import Refresh from '~icons/tabler/refresh';
	import { type useMatrixSettings } from './useMatrixSettings.svelte';
	import * as m from '$lib/paraglide/messages.js';
	import {
		MATRIX_BACKGROUND_MODE_DATA_VISUALIZATION,
		MATRIX_DATA_METRIC_CO2,
		MATRIX_DATA_METRIC_CSI_MOTION,
		MATRIX_DATA_METRIC_HUMIDITY,
		MATRIX_DATA_METRIC_RSSI,
		MATRIX_DATA_METRIC_SIGNAL_QUALITY,
		MATRIX_DATA_METRIC_TEMPERATURE,
		MATRIX_DATA_SOURCE_BLE,
		MATRIX_DATA_SOURCE_SCD4X,
		MATRIX_DATA_SOURCE_WIFI_CSI,
		MATRIX_DATA_SOURCE_WIFI_RSSI,
		MATRIX_DATA_STALE_BLANK,
		MATRIX_DATA_STALE_DIM,
		MATRIX_DATA_STALE_GRAY,
		MATRIX_DATA_VIZ_MODE_CENTER_RIPPLE,
		MATRIX_DATA_VIZ_MODE_GAUGE,
		MATRIX_DATA_VIZ_MODE_HEATMAP,
		MATRIX_DATA_VIZ_MODE_TREND,
		fromMatrixHexColor,
		toMatrixHexColor,
		type MatrixDataVisualizationMetric,
		type MatrixDataVisualizationMode,
		type MatrixDataVisualizationSource,
		type MatrixDataVisualizationStaleBehavior
	} from './matrixModel';

	let {
		store,
		canManage = true
	}: {
		store: ReturnType<typeof useMatrixSettings>;
		canManage?: boolean;
	} = $props();

	const session = useSessionAccess();
	let bleStatus = $state<BleStatus | null>(null);
	let bleLoadError = $state('');
	let calibrationBusy = $state(false);
	let calibrationMessage = $state('');
	let minHex = $state('#0040ff');
	let midHex = $state('#00ff80');
	let maxHex = $state('#ff3000');

	const controlsDisabled = $derived(!canManage || !store.settings.data_visualization_enabled);
	const selectedSource = $derived(
		store.settings.data_visualization_source as MatrixDataVisualizationSource
	);
	const showBleSelector = $derived(selectedSource === MATRIX_DATA_SOURCE_BLE);
	const showCsiCalibration = $derived(selectedSource === MATRIX_DATA_SOURCE_WIFI_CSI);

	$effect(() => {
		if (!store.loading) {
			minHex = toMatrixHexColor(store.settings.data_visualization_color_min);
			midHex = toMatrixHexColor(store.settings.data_visualization_color_mid);
			maxHex = toMatrixHexColor(store.settings.data_visualization_color_max);
		}
	});

	$effect(() => {
		if (session.canRead) {
			void loadBleStatus();
		}
	});

	const sourceOptions = $derived([
		{ value: MATRIX_DATA_SOURCE_SCD4X, label: m.matrix_data_viz_source_scd4x() },
		{ value: MATRIX_DATA_SOURCE_BLE, label: m.matrix_data_viz_source_ble() },
		{ value: MATRIX_DATA_SOURCE_WIFI_RSSI, label: m.matrix_data_viz_source_wifi_rssi() },
		{ value: MATRIX_DATA_SOURCE_WIFI_CSI, label: m.matrix_data_viz_source_wifi_csi() }
	]);

	const modeOptions = $derived([
		{ value: MATRIX_DATA_VIZ_MODE_GAUGE, label: m.matrix_data_viz_mode_gauge() },
		{ value: MATRIX_DATA_VIZ_MODE_CENTER_RIPPLE, label: m.matrix_data_viz_mode_center_ripple() },
		{ value: MATRIX_DATA_VIZ_MODE_HEATMAP, label: m.matrix_data_viz_mode_heatmap() },
		{ value: MATRIX_DATA_VIZ_MODE_TREND, label: m.matrix_data_viz_mode_trend() }
	]);

	const staleOptions = $derived([
		{ value: MATRIX_DATA_STALE_DIM, label: m.matrix_data_viz_stale_dim() },
		{ value: MATRIX_DATA_STALE_GRAY, label: m.matrix_data_viz_stale_gray() },
		{ value: MATRIX_DATA_STALE_BLANK, label: m.matrix_data_viz_stale_blank() }
	]);

	const metricOptions = $derived.by(() => {
		switch (selectedSource) {
			case MATRIX_DATA_SOURCE_BLE:
				return [
					{ value: MATRIX_DATA_METRIC_TEMPERATURE, label: m.matrix_data_viz_metric_temperature() },
					{ value: MATRIX_DATA_METRIC_HUMIDITY, label: m.matrix_data_viz_metric_humidity() },
					{ value: MATRIX_DATA_METRIC_RSSI, label: m.matrix_data_viz_metric_rssi() }
				];
			case MATRIX_DATA_SOURCE_WIFI_RSSI:
				return [
					{ value: MATRIX_DATA_METRIC_RSSI, label: m.matrix_data_viz_metric_rssi() },
					{
						value: MATRIX_DATA_METRIC_SIGNAL_QUALITY,
						label: m.matrix_data_viz_metric_signal_quality()
					}
				];
			case MATRIX_DATA_SOURCE_WIFI_CSI:
				return [{ value: MATRIX_DATA_METRIC_CSI_MOTION, label: m.matrix_data_viz_metric_csi_motion() }];
			case MATRIX_DATA_SOURCE_SCD4X:
			default:
				return [
					{ value: MATRIX_DATA_METRIC_CO2, label: m.matrix_data_viz_metric_co2() },
					{ value: MATRIX_DATA_METRIC_TEMPERATURE, label: m.matrix_data_viz_metric_temperature() },
					{ value: MATRIX_DATA_METRIC_HUMIDITY, label: m.matrix_data_viz_metric_humidity() }
				];
		}
	});

	const bleDeviceOptions = $derived.by(() => [
		{ value: '', label: m.matrix_data_viz_ble_device_placeholder() },
		...(bleStatus?.devices ?? []).map((device) => ({
			value: device.mac,
			label: `${device.mac} - ${device.temp.toFixed(1)}C - ${device.humid.toFixed(0)}%`
		}))
	]);

	function setDefaultMetricForSource(source: MatrixDataVisualizationSource) {
		switch (source) {
			case MATRIX_DATA_SOURCE_BLE:
				store.settings.data_visualization_metric = MATRIX_DATA_METRIC_TEMPERATURE;
				break;
			case MATRIX_DATA_SOURCE_WIFI_RSSI:
				store.settings.data_visualization_metric = MATRIX_DATA_METRIC_RSSI;
				break;
			case MATRIX_DATA_SOURCE_WIFI_CSI:
				store.settings.data_visualization_metric = MATRIX_DATA_METRIC_CSI_MOTION;
				store.settings.data_visualization_mode = MATRIX_DATA_VIZ_MODE_HEATMAP;
				store.settings.data_visualization_min = 0;
				store.settings.data_visualization_max = 100;
				break;
			case MATRIX_DATA_SOURCE_SCD4X:
			default:
				store.settings.data_visualization_metric = MATRIX_DATA_METRIC_CO2;
				break;
		}
	}

	function handleEnabledChange(e: Event) {
		if (!canManage) return;
		const enabled = (e.currentTarget as HTMLInputElement).checked;
		store.settings.data_visualization_enabled = enabled;
		if (enabled) {
			store.settings.background_mode = MATRIX_BACKGROUND_MODE_DATA_VISUALIZATION;
		}
	}

	function handleSourceChange(e: Event) {
		if (!canManage) return;
		const source = Number((e.currentTarget as HTMLSelectElement).value) as MatrixDataVisualizationSource;
		store.settings.data_visualization_source = source;
		store.settings.background_mode = MATRIX_BACKGROUND_MODE_DATA_VISUALIZATION;
		setDefaultMetricForSource(source);
	}

	function handleMetricChange(e: Event) {
		if (!canManage) return;
		store.settings.data_visualization_metric = Number(
			(e.currentTarget as HTMLSelectElement).value
		) as MatrixDataVisualizationMetric;
	}

	function handleModeChange(e: Event) {
		if (!canManage) return;
		store.settings.data_visualization_mode = Number(
			(e.currentTarget as HTMLSelectElement).value
		) as MatrixDataVisualizationMode;
	}

	function handleStaleChange(e: Event) {
		if (!canManage) return;
		store.settings.data_visualization_stale_behavior = Number(
			(e.currentTarget as HTMLSelectElement).value
		) as MatrixDataVisualizationStaleBehavior;
	}

	function handleColorMin(e: Event) {
		if (!canManage) return;
		minHex = (e.target as HTMLInputElement).value;
		store.settings.data_visualization_color_min = fromMatrixHexColor(minHex);
	}

	function handleColorMid(e: Event) {
		if (!canManage) return;
		midHex = (e.target as HTMLInputElement).value;
		store.settings.data_visualization_color_mid = fromMatrixHexColor(midHex);
	}

	function handleColorMax(e: Event) {
		if (!canManage) return;
		maxHex = (e.target as HTMLInputElement).value;
		store.settings.data_visualization_color_max = fromMatrixHexColor(maxHex);
	}

	async function loadBleStatus() {
		try {
			bleLoadError = '';
			bleStatus = await new BleApiService(session.apiOptions).getStatus();
		} catch {
			bleLoadError = m.matrix_data_viz_ble_load_error({ locale: i18n.languageTag });
		}
	}

	async function calibrateCsi() {
		if (!canManage || calibrationBusy) return;
		calibrationBusy = true;
		calibrationMessage = '';
		try {
			await new MatrixApiService(session.apiOptions).calibrateCsiDataVisualization();
			calibrationMessage = m.matrix_data_viz_csi_calibrate_ok({ locale: i18n.languageTag });
		} catch {
			calibrationMessage = m.matrix_data_viz_csi_calibrate_error({ locale: i18n.languageTag });
		} finally {
			calibrationBusy = false;
		}
	}
</script>

<SettingsCard
	title={m.matrix_data_viz_title()}
	icon={ChartBar}
	hasChanges={store.hasChanges}
	loading={store.loading}
	saving={store.saving}
	disabled={!canManage}
	error={store.error}
	onSave={store.saveSettingsNow}
	onReset={store.resetSettings}
	dirtySourceId="matrix-data-visualization"
>
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
			<ContentBox class="flex items-center justify-between">
				<div>
					<div class="flex flex-wrap items-center gap-2">
						<span class="font-bold text-sm">{m.matrix_data_viz_enable()}</span>
						<span
							class="badge badge-sm {store.settings.data_visualization_enabled
								? 'badge-success'
								: 'badge-ghost'}"
						>
							{store.settings.data_visualization_enabled
								? m.imu_state_enabled({ locale: i18n.languageTag })
								: m.imu_state_disabled({ locale: i18n.languageTag })}
						</span>
					</div>
					<p class="text-xs text-base-content/70">{m.matrix_data_viz_desc()}</p>
				</div>
				<FormToggle
					label=""
					bind:checked={store.settings.data_visualization_enabled}
					disabled={!canManage}
					ariaLabel={m.matrix_data_viz_enable({ locale: i18n.languageTag })}
					plain={true}
					onchange={handleEnabledChange}
				/>
			</ContentBox>

			<div class="flex flex-col gap-1" aria-disabled={controlsDisabled}>
				<ContentBox>
					<FormSelect
						label={m.matrix_data_viz_source()}
						value={store.settings.data_visualization_source}
						options={sourceOptions}
						disabled={controlsDisabled}
						onchange={handleSourceChange}
					/>
				</ContentBox>

				<ContentBox>
					<FormSelect
						label={m.matrix_data_viz_metric()}
						value={store.settings.data_visualization_metric}
						options={metricOptions}
						disabled={controlsDisabled}
						onchange={handleMetricChange}
					/>
				</ContentBox>

				{#if showBleSelector}
					<ContentBox>
						<div class="flex items-end gap-2">
							<div class="min-w-0 flex-1">
								<FormSelect
									label={m.matrix_data_viz_ble_device()}
									value={store.settings.data_visualization_device_id}
									options={bleDeviceOptions}
									disabled={controlsDisabled}
									onchange={(e) =>
										(store.settings.data_visualization_device_id = String(
											(e.currentTarget as HTMLSelectElement).value
										))}
								/>
							</div>
							<FormButton
								variant="ghost"
								size="sm"
								icon={Refresh}
								disabled={!canManage}
								ariaLabel={m.tooltip_refresh({ locale: i18n.languageTag })}
								onclick={() => void loadBleStatus()}
							/>
						</div>
						{#if bleLoadError}
							<p class="mt-2 text-xs text-warning">{bleLoadError}</p>
						{/if}
					</ContentBox>
				{/if}

				<ContentBox>
					<FormSelect
						label={m.matrix_data_viz_mode()}
						value={store.settings.data_visualization_mode}
						options={modeOptions}
						disabled={controlsDisabled}
						onchange={handleModeChange}
					/>
				</ContentBox>

				<ContentBox>
					<div class="grid gap-3 sm:grid-cols-2">
						<FormInput
							type="number"
							step="0.1"
							label={m.matrix_data_viz_min()}
							bind:value={store.settings.data_visualization_min}
							disabled={controlsDisabled}
						/>
						<FormInput
							type="number"
							step="0.1"
							label={m.matrix_data_viz_max()}
							bind:value={store.settings.data_visualization_max}
							disabled={controlsDisabled}
						/>
					</div>
				</ContentBox>

				<ContentBox>
					<div class="flex items-center justify-between gap-3">
						<span class="font-bold text-sm">{m.matrix_data_viz_colors()}</span>
						<div class="flex flex-col gap-1">
							<div class="flex items-center justify-end gap-2">
								<span class="font-mono text-xs opacity-70">{minHex}</span>
								<FormColorInput
									value={minHex}
									disabled={controlsDisabled}
									ariaLabel={m.matrix_data_viz_color_min({ locale: i18n.languageTag })}
									oninput={handleColorMin}
								/>
							</div>
							<div class="flex items-center justify-end gap-2">
								<span class="font-mono text-xs opacity-70">{midHex}</span>
								<FormColorInput
									value={midHex}
									disabled={controlsDisabled}
									ariaLabel={m.matrix_data_viz_color_mid({ locale: i18n.languageTag })}
									oninput={handleColorMid}
								/>
							</div>
							<div class="flex items-center justify-end gap-2">
								<span class="font-mono text-xs opacity-70">{maxHex}</span>
								<FormColorInput
									value={maxHex}
									disabled={controlsDisabled}
									ariaLabel={m.matrix_data_viz_color_max({ locale: i18n.languageTag })}
									oninput={handleColorMax}
								/>
							</div>
						</div>
					</div>
				</ContentBox>

				<ContentBox>
					<FormRange
						label={m.matrix_data_viz_brightness_min()}
						min={0}
						max={255}
						step={1}
						valueClass="w-12"
						disabled={controlsDisabled}
						bind:value={store.settings.data_visualization_brightness_min}
					/>
					<FormRange
						class="mt-3"
						label={m.matrix_data_viz_brightness_max()}
						min={0}
						max={255}
						step={1}
						valueClass="w-12"
						disabled={controlsDisabled}
						bind:value={store.settings.data_visualization_brightness_max}
					/>
				</ContentBox>

				<ContentBox>
					<FormRange
						label={m.matrix_data_viz_smoothing()}
						min={0}
						max={100}
						step={1}
						valueClass="w-12"
						disabled={controlsDisabled}
						bind:value={store.settings.data_visualization_smoothing}
					/>
				</ContentBox>

				<ContentBox>
					<FormSelect
						label={m.matrix_data_viz_stale_behavior()}
						value={store.settings.data_visualization_stale_behavior}
						options={staleOptions}
						disabled={controlsDisabled}
						onchange={handleStaleChange}
					/>
				</ContentBox>

				{#if showCsiCalibration}
					<ContentBox class="flex items-center justify-between gap-3">
						<div class="min-w-0">
							<span class="font-bold text-sm">{m.matrix_data_viz_csi_calibrate()}</span>
							{#if calibrationMessage}
								<p class="mt-1 text-xs text-base-content/70">{calibrationMessage}</p>
							{/if}
						</div>
						<FormButton
							variant="secondary"
							size="sm"
							icon={Refresh}
							label={calibrationBusy
								? m.matrix_data_viz_csi_calibrating()
								: m.matrix_data_viz_csi_calibrate()}
							disabled={controlsDisabled}
							loading={calibrationBusy}
							onclick={() => void calibrateCsi()}
						/>
					</ContentBox>
				{/if}
			</div>
		{/if}
	</div>
</SettingsCard>
