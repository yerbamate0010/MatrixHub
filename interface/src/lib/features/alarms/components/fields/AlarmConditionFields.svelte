<script lang="ts">
	import type { AlarmSource, AlarmOperator } from '$lib/types/domain/alarms';
	import { ALARM_SOURCES } from '$lib/types/domain/alarms';
	import { i18n } from '$lib/i18n.svelte';
	import * as m from '$lib/paraglide/messages.js';

	import { FormSelect } from '$lib/components/shared/forms';

	let {
		source = $bindable(),
		operator = $bindable(),
		onSourceChange,
		onOperatorChange
	}: {
		source: AlarmSource;
		operator: AlarmOperator;
		onSourceChange?: (source: AlarmSource) => void;
		onOperatorChange?: (operator: AlarmOperator) => void;
	} = $props();

	function getSourceLabel(source: string) {
		switch (source) {
			case 'co2':
				return m.source_co2({ locale: i18n.languageTag });
			case 'temperature':
				return m.source_temperature({ locale: i18n.languageTag });
			case 'humidity':
				return m.source_humidity({ locale: i18n.languageTag });
			case 'wifi_motion':
				return m.source_wifi_motion({ locale: i18n.languageTag });
			case 'ble_temperature':
				return m.source_ble_temperature({ locale: i18n.languageTag });
			case 'ble_humidity':
				return m.source_ble_humidity({ locale: i18n.languageTag });
			default:
				return source;
		}
	}

	let sourceOptions = $derived(
		Object.keys(ALARM_SOURCES).map((key) => ({
			value: key,
			label: getSourceLabel(key)
		}))
	);

	let operatorOptions = $derived([
		{ value: 'above', label: m.alarms_operator_above({ locale: i18n.languageTag }) },
		{ value: 'below', label: m.alarms_operator_below({ locale: i18n.languageTag }) }
	]);

	function handleSourceSelect(newSource: AlarmSource) {
		source = newSource;
		onSourceChange?.(newSource);
	}

	function handleOperatorSelect(newOperator: AlarmOperator) {
		operator = newOperator;
		onOperatorChange?.(newOperator);
	}
</script>

<div class="flex flex-row items-center gap-2 w-full">
	<div class="grow min-w-0">
		<FormSelect
			value={source}
			options={sourceOptions}
			onchange={(e) => handleSourceSelect((e.target as HTMLSelectElement).value as AlarmSource)}
		/>
	</div>
	<div class="w-32 flex-none">
		<FormSelect
			value={operator}
			options={operatorOptions}
			onchange={(e) => handleOperatorSelect((e.target as HTMLSelectElement).value as AlarmOperator)}
		/>
	</div>
</div>
