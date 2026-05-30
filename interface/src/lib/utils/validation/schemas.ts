import { z } from 'zod';
import type { SystemInformation } from '$lib/types/system/system';
import { WifiNetworkSchema } from '$lib/features/wifi/wifiValidation';

export { WifiNetworkSchema };

export const SystemInformationSchema = z.object({
	esp_platform: z.string(),
	firmware_version: z.string(),
	firmware_name: z.string().optional(),
	firmware_built_target: z.string().optional(),
	cpu_freq_mhz: z.number(),
	cpu_type: z.string(),
	cpu_rev: z.number(),
	cpu_cores: z.number(),
	sketch_size: z.number(),
	free_sketch_space: z.number(),
	sdk_version: z.string(),
	arduino_version: z.string(),
	flash_chip_size: z.number(),
	flash_chip_speed: z.number(),
	cpu_reset_reason: z.number(),
	max_alloc_heap: z.number(),
	psram_size: z.number(),
	free_psram: z.number(),
	used_psram: z.number(),
	free_heap: z.number(),
	used_heap: z.number(),
	total_heap: z.number(),
	min_free_heap: z.number(),
	core_temp: z.number(),
	fs_total: z.number(),
	fs_used: z.number(),
	lp_sram_used: z.number().optional(),
	lp_sram_free: z.number().optional(),
	lp_sram_total: z.number().optional(),
	uptime: z.number(),
	mac_address: z.string().optional(),
	compile_date: z.string().optional(),
	compile_time: z.string().optional()
}) satisfies z.ZodType<SystemInformation>;

export const WifiRecoveryResponseSchema = z.object({
	success: z.boolean(),
	accepted: z.boolean(),
	connected: z.boolean(),
	ip: z.string().optional(),
	rssi: z.number().optional()
});

export const BleStatusSchema = z.object({
	enabled: z.boolean(),
	running: z.boolean(),
	scanner_active: z.boolean().optional(),
	devices: z
		.array(
			z.object({
				mac: z.string(),
				temp: z.number(),
				humid: z.number(),
				batt: z.number(),
				rssi: z.number(),
				last_seen: z.number(),
				saved: z.boolean().optional()
			})
		)
		.optional()
});

export const BleSettingsSchema = z.object({
	enabled: z.boolean(),
	sensors: z
		.array(
			z.object({
				mac: z.string(),
				alias: z.string()
			})
		)
		.optional()
});
