import { describe, expect, it } from 'vitest';
import { WifiNetworkSchema } from './schemas';

describe('WifiNetworkSchema', () => {
	it('accepts SSIDs up to 32 UTF-8 bytes', () => {
		const asciiResult = WifiNetworkSchema.safeParse({
			ssid: '12345678901234567890123456789012',
			password: ''
		});
		const multibyteResult = WifiNetworkSchema.safeParse({
			ssid: 'ą'.repeat(16),
			password: ''
		});

		expect(asciiResult.success).toBe(true);
		expect(multibyteResult.success).toBe(true);
	});

	it('rejects SSIDs that exceed 32 UTF-8 bytes even when character count looks valid', () => {
		const result = WifiNetworkSchema.safeParse({
			ssid: 'ą'.repeat(17),
			password: ''
		});

		expect(result.success).toBe(false);
		if (result.success) {
			return;
		}

		expect(result.error.flatten().fieldErrors.ssid).toContain('SSID too long (max 32 bytes)');
	});
});
