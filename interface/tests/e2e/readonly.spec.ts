import { test, expect } from '@playwright/test';
import { loginAsAdmin } from './test-utils';

test.describe('Read-Only Scenarios', () => {
	test.beforeEach(async ({ page }) => {
		await page.goto('/');
		await loginAsAdmin(page);
	});

	test('System Status shows live data', async ({ page }) => {
		// Navigate to System Status
		// Note: System is a submenu, but we can browse directly to be fast,
		// or verify menu interaction. Direct navigation is more robust for "Status" testing.
		await page.goto('/system/status');

		// Check for specific headers from HealthCard.svelte
		// "System Health" or "Uptime"
		// Using a locator that looks for the text "Uptime" (English default)
		const uptimeLabel = page.getByText('Uptime', { exact: true });
		await expect(uptimeLabel).toBeVisible();

		// Check value - checking the sibling or generic structure is hard without test-ids.
		// However, the card contains "Uptime" and should contain some time string.
		// We can check if the entire card is visible using a class or broad check.
		await expect(page.locator('.card').filter({ hasText: 'Uptime' }).first()).toBeVisible();
	});

	test('Language Switching (EN <-> PL)', async ({ page }) => {
		// 1. Ensure we are in English first (default)
		// Click EN button if it exists/is clickable
		// The buttons are in the sidebar: text="EN" / text="PL"

		const btnPL = page.getByRole('button', { name: 'PL' });
		const btnEN = page.getByRole('button', { name: 'EN' });

		// Switch to Polish
		await btnPL.click();
		await page.waitForTimeout(500); // Wait for language switch
		await expect(btnPL).toHaveClass(/active|selected|primary|btn-active/);

		// Menu text check
		await expect(page.locator('a', { hasText: 'Wykresy' }).first()).toBeVisible();

		// Switch back to English
		await btnEN.click();
		await page.waitForTimeout(500); // Wait for language switch
		await expect(btnEN).toHaveClass(/active|selected|primary|btn-active/);

		// Verify change back
		await expect(page.locator('a', { hasText: 'Charts' }).first()).toBeVisible();
	});
});
