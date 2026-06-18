/**
 * WiFi Sensing types shared between components
 */

export interface WifiSensingStats {
	current: number;
	filtered?: number;
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

export interface CsiRuntimeMetrics {
	enabled: boolean;
	queue_allocated: boolean;
	active_consumer_mask: number;
	consumer_count: number;
	frontend_consumer_active: boolean;
	alarm_consumer_active: boolean;
	boot_consumer_active: boolean;
	queue_depth: number;
	queue_capacity: number;
	queue_drops_total: number;
	queue_drops_last_sec: number;
	rx_frames_total: number;
	rx_accepted_total: number;
	rx_throttled_total: number;
	queued_packets_total: number;
	dequeued_packets_total: number;
	packets_forwarded_total: number;
	batches_forwarded_total: number;
	batches_dropped_total: number;
	packets_per_sec: number;
	batches_per_sec: number;
	last_packet_ms: number;
	last_batch_ms: number;
	calibration_count: number;
	calibration_target: number;
	calibration_state: string;
	ws_client_count: number;
	ws_queue_enabled: boolean;
}

export interface WifiSensingStatus extends WifiSensingData {
	schema: 'wifisensing.status.v1';
	sample_interval_ms: number;
	csi: CsiRuntimeMetrics;
}

export interface RssiSample {
	rssi: number;
	timestamp: number;
	variance?: number;
}
