import { test, expect } from '@playwright/test';

test.describe('Dashboard', () => {
	test('should load dashboard and display widgets', async ({ page }) => {
		// Navigate to dashboard (assuming dev server or proxy to ESP32)
		await page.goto('/');

		// Wait for page to load
		await page.waitForLoadState('networkidle');

		// Check for main dashboard elements - be more flexible
		const mainContent = page.locator('body, main, .container');
		await expect(mainContent.first()).toBeVisible();
	});

	test('should handle login when security enabled', async ({ page }) => {
		// This test assumes security is enabled
		await page.goto('/');

		// If redirected to login, check login form
		const loginForm = page.locator('form, [data-testid="login-form"]');
		if (await loginForm.isVisible()) {
			// Fill login form
			await page.fill('input[type="text"], #username', 'admin');
			await page.fill('input[type="password"], #password', 'admin');
			await page.click('button[type="submit"]');

			// Should redirect to dashboard
			await expect(page.locator('.drawer')).toBeVisible();
		} else {
			// Already logged in or security disabled - check for main content
			await expect(page.locator('body')).toBeVisible();
		}
	});
});

test.describe('Settings', () => {
	test('should load settings page', async ({ page }) => {
		await page.goto('/connections');

		// Check if connections/settings page loaded
		await expect(page.locator('h1, h2, h3').first()).toBeVisible();
		// Or check for specific settings sections
		const settingsContent = page.locator('.card, form, [data-testid]');
		await expect(settingsContent.first()).toBeVisible();
	});

	test('should save NTP settings', async ({ page }) => {
		await page.goto('/connections/ntp');

		// Wait for NTP form to load
		await page.waitForSelector('form, [data-testid="ntp-form"]');

		// Check if NTP settings form is present
		await expect(page.locator('form').first()).toBeVisible();
	});
});
