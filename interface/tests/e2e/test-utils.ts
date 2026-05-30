import { type Page, expect } from '@playwright/test';

export async function loginAsAdmin(page: Page) {
	const username = process.env.TEST_USERNAME ?? 'admin';
	const password = process.env.TEST_PASSWORD ?? 'admin';

	// Wait for either the main layout OR the login form
	const drawer = page.locator('.drawer');
	const loginForm = page.locator('[data-testid="login-username"], #user');

	try {
		await expect(drawer.or(loginForm)).toBeVisible({ timeout: 10000 });
	} catch (e) {
		console.log('Timeout waiting for drawer or login form');
		throw e;
	}

	if (await loginForm.isVisible()) {
		await page.fill('[data-testid="login-username"], #user', username);
		await page.fill('[data-testid="login-password"], #pwd', password);
		await page.click('[data-testid="login-submit"], button[type="submit"]');
		await expect(drawer).toBeVisible({ timeout: 15000 });
	}
}
