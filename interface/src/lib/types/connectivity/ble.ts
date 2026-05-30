/**
 * BLE (Bluetooth Low Energy) Types
 */

export interface BleStatus {
	enabled: boolean;
	running: boolean;
	scanner_active?: boolean;
	devices?: Array<{
		mac: string;
		temp: number;
		humid: number;
		batt: number;
		rssi: number;
		last_seen: number;
		saved?: boolean;
	}>;
}

export interface BleSensorConfig {
	mac: string;
	alias: string;
}

export interface BleSettings {
	enabled: boolean;
	sensors?: BleSensorConfig[];
}
