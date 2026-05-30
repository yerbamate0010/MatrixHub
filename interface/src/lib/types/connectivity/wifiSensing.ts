/**
 * WiFi Sensing types shared between components
 */

export interface WifiSensingStats {
	current: number;
	min: number;
	max: number;
	avg: number;
	variance: number;
	sampleCount: number;
	windowMs: number;
}

export interface WifiSensingData {
	enabled: boolean;
	running: boolean;
	active: boolean;
	connectedSSID: string;
	connectedChannel: number;
	stats: WifiSensingStats;
	motionDetected: boolean;
	variance_threshold: number;
	samples?: RssiSample[];
}

export interface WifiSensingSettings {
	enabled: boolean;
	sample_interval_ms: number;
	variance_threshold: number;
}

export interface RssiSample {
	rssi: number;
	timestamp: number;
	variance?: number;
}
