import { test, expect, type Page, type Route } from '@playwright/test';

type MatrixSettings = {
	brightness: number;
	alarm_mode: number;
	rotation: number;
	auto_rotate: boolean;
	effect_enabled: boolean;
	effect_mode: number;
	effect_speed: number;
	effect_color: number;
	effect_color_2: number;
	effect_color_3: number;
	menu_enabled: boolean;
	menu_text_color: number;
	menu_scroll_speed: number;
	custom_icons?: number[][];
};

const initialMatrixSettings: MatrixSettings = {
	brightness: 20,
	alarm_mode: 1,
	rotation: 0,
	auto_rotate: false,
	effect_enabled: true,
	effect_mode: 2,
	effect_speed: 1000,
	effect_color: 0x00ff00,
	effect_color_2: 0xff0000,
	effect_color_3: 0x0000ff,
	menu_enabled: true,
	menu_text_color: 0xffffff,
	menu_scroll_speed: 20,
	custom_icons: [[], [], []]
};

function createTestToken() {
	const header = Buffer.from(JSON.stringify({ alg: 'none', typ: 'JWT' })).toString('base64url');
	const payload = Buffer.from(JSON.stringify({ username: 'admin', admin: true })).toString(
		'base64url'
	);
	return `${header}.${payload}.matrixhub-e2e`;
}

async function fulfillJson(route: Route, payload: unknown, status = 200) {
	await route.fulfill({
		status,
		contentType: 'application/json',
		body: JSON.stringify(payload)
	});
}

async function installAuthenticatedSession(page: Page) {
	const token = createTestToken();
	await page.addInitScript((accessToken) => {
		window.localStorage.setItem(
			'user',
			JSON.stringify({
				username: 'admin',
				admin: true,
				bearer_token: accessToken
			})
		);
	}, token);
}

test.describe('Matrix LED settings', () => {
	test('saves matrix display, effect, color, and icon drafts through the card Save action', async ({
		page
	}) => {
		let matrixSettings = structuredClone(initialMatrixSettings);
		const matrixPosts: MatrixSettings[] = [];

		await installAuthenticatedSession(page);
		await page.route('**/rest/features', (route) =>
			fulfillJson(route, { ntp: true, sleep: true, file_manager: true })
		);
		await page.route('**/api/matrix/settings', async (route) => {
			if (route.request().method() === 'GET') {
				await fulfillJson(route, matrixSettings);
				return;
			}

			if (route.request().method() === 'POST') {
				const payload = (route.request().postDataJSON() ?? {}) as Partial<MatrixSettings>;
				matrixSettings = { ...matrixSettings, ...payload };
				matrixPosts.push(structuredClone(matrixSettings));
				await fulfillJson(route, matrixSettings);
				return;
			}

			await route.continue();
		});

		await page.goto('/system/matrix');

		await expect(page.getByRole('heading', { name: 'Matrix LED Settings' })).toBeVisible({
			timeout: 30_000
		});
		await expect(page.getByRole('heading', { name: 'Visual Effects' })).toBeVisible({
			timeout: 30_000
		});

		const brightnessSlider = page.locator('input[type="range"]').nth(1);
		await brightnessSlider.evaluate((input) => {
			const range = input as HTMLInputElement;
			range.value = '88';
			range.dispatchEvent(new Event('input', { bubbles: true }));
		});
		await page.getByRole('button', { name: '90°' }).click();
		await page.getByLabel('Menu Text Color').fill('#112233');
		await page.getByRole('combobox', { name: 'Effect Mode' }).selectOption('11');

		await page.getByRole('button', { name: 'Edit Icons' }).click();
		const iconDialog = page.getByRole('dialog');
		await iconDialog.getByRole('button', { name: 'Pixel 1', exact: true }).click();
		await iconDialog.getByRole('button', { name: 'Save' }).click();

		expect(matrixPosts).toHaveLength(0);

		await page.getByRole('button', { name: 'Save' }).first().click();
		await expect.poll(() => matrixPosts.length).toBe(1);

		expect(matrixPosts[0]).toMatchObject({
			brightness: 88,
			rotation: 1,
			menu_text_color: 0x112233,
			effect_mode: 11,
			menu_enabled: true
		});
		expect(matrixPosts[0].custom_icons?.[0]).toHaveLength(64);

		await page.reload();
		await expect(page.getByRole('heading', { name: 'Matrix LED Settings' })).toBeVisible({
			timeout: 30_000
		});
		await expect(page.locator('input[type="range"]').nth(1)).toHaveValue('88');
		await expect(page.getByLabel('Menu Text Color')).toHaveValue('#112233');
		await expect(page.getByRole('combobox', { name: 'Effect Mode' })).toHaveValue('11');
	});

	test('guards unsaved matrix drafts during internal navigation', async ({ page }) => {
		let matrixSettings = structuredClone(initialMatrixSettings);

		await installAuthenticatedSession(page);
		await page.route('**/rest/features', (route) =>
			fulfillJson(route, { ntp: true, sleep: true, file_manager: true })
		);
		await page.route('**/api/matrix/settings', async (route) => {
			if (route.request().method() === 'GET') {
				await fulfillJson(route, matrixSettings);
				return;
			}

			if (route.request().method() === 'POST') {
				const payload = (route.request().postDataJSON() ?? {}) as Partial<MatrixSettings>;
				matrixSettings = { ...matrixSettings, ...payload };
				await fulfillJson(route, matrixSettings);
				return;
			}

			await route.continue();
		});

		await page.goto('/system/matrix');
		await expect(page.getByRole('heading', { name: 'Matrix LED Settings' })).toBeVisible({
			timeout: 30_000
		});

		const brightnessSlider = page.locator('input[type="range"]').nth(1);
		await brightnessSlider.evaluate((input) => {
			const range = input as HTMLInputElement;
			range.value = '77';
			range.dispatchEvent(new Event('input', { bubbles: true }));
		});

		const dismissedDialogs: string[] = [];
		page.once('dialog', async (dialog) => {
			dismissedDialogs.push(dialog.message());
			await dialog.dismiss();
		});
		await page.getByRole('link', { name: 'Charts' }).click();
		await expect(page).toHaveURL(/\/system\/matrix$/);
		expect(dismissedDialogs[0]).toContain('unsaved changes');

		page.once('dialog', async (dialog) => {
			await dialog.accept();
		});
		await page.getByRole('link', { name: 'Charts' }).click();
		await expect(page).toHaveURL(/\/charts$/);
	});
});
