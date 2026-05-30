import { test, expect } from '@playwright/test';

const username = process.env.TEST_USERNAME ?? 'admin';
const password = process.env.TEST_PASSWORD ?? 'admin';

test('smoke: app renders (and can login when security enabled)', async ({ page }) => {
	await page.goto('/');

	// Wait for either the main layout OR the login form
	// We use a race to see which one appears first
	const drawer = page.locator('.drawer');
	const loginForm = page.locator('[data-testid="login-username"], #user');

	try {
		await expect(drawer.or(loginForm)).toBeVisible({ timeout: 10000 });
	} catch (e) {
		console.log('Timeout waiting for drawer or login form');
		// Take a screenshot if possible or just fail
		throw e;
	}

	if (await loginForm.isVisible()) {
		await page.fill('[data-testid="login-username"], #user', username);
		await page.fill('[data-testid="login-password"], #pwd', password);
		await page.click('[data-testid="login-submit"], button[type="submit"]');

		// Wait for navigation/UI update
		await expect(drawer).toBeVisible({ timeout: 15000 });
	} else {
		// No login form detected, assuming already logged in or security disabled.
		await expect(drawer).toBeVisible();
	}

	// Verify navigation to Charts
	const chartsLink = page.locator('a[href="/charts"]').first();

	// Ensure header/menu is visible; on smaller screens checking visibility might need menu toggle
	if (await chartsLink.isVisible()) {
		await chartsLink.click();
	} else {
		// Possibly inside mobile menu
		await page.goto('/charts');
	}

	await expect(page).toHaveURL(/.*\/charts/);

	await expect(page.locator('.drawer-content')).toBeVisible();
});
