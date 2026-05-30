export interface PowerConfig {
	sleep_enabled: boolean;
	inactivity_timeout_ms: number;
	grace_after_boot_ms: number;
}

export interface PowerStatus extends PowerConfig {
	wake_reason: string;
	wake_cause_raw: number;
	wake_gpio_mask: string;
	wake_ext1_mask: string;
	sleep_requested: boolean;
	sleep_eta_ms: number;
	wake_interval_ms: number;
	last_activity_ms: number;
	uptime_ms: number;
}
