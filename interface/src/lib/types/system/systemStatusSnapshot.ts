import type { WifiSettings } from '$lib/types/connectivity/wifi';
import type { SystemInformation } from '$lib/types/system/system';
import type { HealthDiagnostics } from '$lib/types/system/health';

// Older firmware snapshots still send the full wifi_settings object. The
// dashboard now only needs AP-mode derivation, but keeping this narrow legacy
// shape in the type lets staggered FE/BE deploys degrade gracefully.
type LegacySystemStatusWifiSettings = Pick<WifiSettings, 'connection_mode' | 'wifi_networks'>;

export type SystemStatusWifiDiagnostics = HealthDiagnostics['wifi'] & {
	lastDisconnectReason: number;
	healthy: boolean;
	state?: string;
	mode?: string;
	rescueApActive?: boolean;
	rescueReason?: string;
	lastRecoveryReason?: string;
	lastIpChangeMs?: number;
	disconnectedSinceMs?: number;
	stableConnectedSinceMs?: number;
	savedStaticIp?: string;
	mac?: string;
	channel?: number;
	bssid?: string;
	gateway?: string;
	subnet?: string;
	dns?: string;
};

export interface SystemStatusApDiagnostics {
	active?: boolean;
	mode?: string;
	stationNum?: number;
	ip?: string;
	mac?: string;
}

export interface SystemStatusHttpDiagnostics {
	activeClients?: number;
	peakClients?: number;
	opens?: number;
	closes?: number;
	lastOpenMs?: number;
	lastCloseMs?: number;
	wsForcedRemovals?: number;
	wsQueueDrops?: number;
	lastWsQueueDropMs?: number;
	lastWsQueueDropPayload?: number;
	wsHeapFallbacks?: number;
	lastWsHeapFallbackMs?: number;
	lastWsHeapFallbackPayload?: number;
	maxWsHeapFallbackPayload?: number;
}

export interface SystemStatusForwardingDiagnostics {
	ready?: boolean;
	requiresStaticIp?: boolean;
	savedStaticIpConfigured?: boolean;
	savedStaticIpMatches?: boolean;
	httpsPort?: number;
}

export interface DashboardBleWidgetSummary {
	enabled: boolean;
	sensor_count: number;
}

export interface DashboardShellyWidgetSummary {
	device_count: number;
}

export interface DashboardAlarmsWidgetSummary {
	rule_count: number;
}

export interface DashboardWifiSensingWidgetSummary {
	enabled: boolean;
}

export interface SystemStatusDashboardWidgetsSummary {
	ble: DashboardBleWidgetSummary;
	shelly: DashboardShellyWidgetSummary;
	alarms: DashboardAlarmsWidgetSummary;
	wifi_sensing: DashboardWifiSensingWidgetSummary;
}

export interface ExtendedHealthDiagnostics extends Omit<HealthDiagnostics, 'wifi'> {
	wifi: SystemStatusWifiDiagnostics;
	ap?: SystemStatusApDiagnostics;
	http?: SystemStatusHttpDiagnostics;
	forwarding?: SystemStatusForwardingDiagnostics;
}

export interface SystemStatusSnapshot {
	system_info?: SystemInformation;
	diagnostics?: ExtendedHealthDiagnostics;
	dashboard_widgets?: SystemStatusDashboardWidgetsSummary;
	wifi_ap_mode?: boolean;
	wifi_settings?: LegacySystemStatusWifiSettings;
	config?: { logging?: { level?: string } };
}
