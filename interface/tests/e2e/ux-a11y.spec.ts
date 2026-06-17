import { test, expect, type Page, type Route } from '@playwright/test';

type JsonObject = Record<string, unknown>;

const matrixSettings = {
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

const notificationSettings = {
	telegram_enabled: false,
	webhook_enabled: false,
	bot_token: '',
	chat_id: '',
	commands_enabled: false,
	webhook_url: '',
	pushover_enabled: false,
	pushover_user: '',
	pushover_token: '',
	is_configured: false
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

function apiPayload(pathname: string): JsonObject {
	switch (pathname) {
		case '/rest/features':
			return { ntp: true, sleep: true, file_manager: true };
		case '/api/matrix/settings':
			return matrixSettings;
		case '/rest/power/status':
			return {
				sleep_enabled: false,
				inactivity_timeout_ms: 600000,
				grace_after_boot_ms: 30000,
				wake_reason: 'poweron',
				wake_cause_raw: 0,
				wake_gpio_mask: '0x0',
				wake_ext1_mask: '0x0',
				sleep_requested: false,
				sleep_eta_ms: 0,
				wake_interval_ms: 0,
				last_activity_ms: 1000,
				uptime_ms: 120000
			};
		case '/rest/wifiStatus':
			return {
				status: 3,
				local_ip: '192.168.1.50',
				mac_address: 'AA:BB:CC:DD:EE:FF',
				rssi: -55,
				ssid: 'MatrixHub',
				bssid: '11:22:33:44:55:66',
				channel: 6,
				subnet_mask: '255.255.255.0',
				gateway_ip: '192.168.1.1',
				dns_ip_1: '1.1.1.1',
				dns_ip_2: '8.8.8.8'
			};
		case '/rest/wifiSettings':
			return {
				hostname: 'matrixhub',
				mode: 'sta',
				wifi_networks: [
					{
						ssid: 'MatrixHub',
						password: 'password123',
						static_ip_config: false
					}
				]
			};
		case '/rest/listNetworks':
			return { networks: [], scan_state: 'idle' };
		case '/api/notifications/settings':
			return notificationSettings;
		case '/api/config':
			return {};
		case '/api/system/info':
			return {
				hostname: 'matrixhub',
				firmware: 'test',
				version: 'e2e',
				filesystem_total: 1024,
				filesystem_used: 256
			};
		default:
			return {};
	}
}

async function installDeviceApiMocks(page: Page) {
	await page.route(/^https?:\/\/[^/]+\/(api|rest)\//, async (route) => {
		const request = route.request();
		const pathname = new URL(request.url()).pathname;
		let postBody: JsonObject = {};
		try {
			postBody = (request.postDataJSON() ?? {}) as JsonObject;
		} catch {
			postBody = {};
		}
		const payload =
			request.method() === 'POST' ? { ...apiPayload(pathname), ...postBody } : apiPayload(pathname);
		await fulfillJson(route, payload);
	});
}

async function expectNoHorizontalOverflow(page: Page) {
	const overflow = await page.evaluate(() => {
		const root = document.documentElement;
		const offenders = Array.from(document.body.querySelectorAll<HTMLElement>('*'))
			.filter((element) => {
				const rect = element.getBoundingClientRect();
				const style = window.getComputedStyle(element);
				return (
					style.display !== 'none' &&
					style.visibility !== 'hidden' &&
					rect.width > 0 &&
					rect.right > window.innerWidth + 1
				);
			})
			.slice(0, 5)
			.map((element) => ({
				tag: element.tagName.toLowerCase(),
				className: element.className.toString().slice(0, 120),
				right: Math.round(element.getBoundingClientRect().right)
			}));

		return {
			scrollWidth: root.scrollWidth,
			clientWidth: root.clientWidth,
			offenders
		};
	});

	expect(overflow, JSON.stringify(overflow.offenders, null, 2)).toEqual(
		expect.objectContaining({
			scrollWidth: expect.any(Number),
			clientWidth: expect.any(Number)
		})
	);
	expect(overflow.scrollWidth).toBeLessThanOrEqual(overflow.clientWidth + 1);
}

async function expectInteractiveControlsHaveNames(page: Page) {
	const unnamedControls = await page.evaluate(() => {
		function hasAssociatedLabel(element: HTMLElement) {
			const id = element.id;
			if (id && document.querySelector(`label[for="${CSS.escape(id)}"]`)) return true;
			if (element.closest('label')) return true;
			const ariaLabelledBy = element.getAttribute('aria-labelledby');
			return Boolean(ariaLabelledBy && document.getElementById(ariaLabelledBy));
		}

		function isVisibleControl(element: HTMLElement) {
			if (element.closest('[hidden], [aria-hidden="true"]')) return false;
			const style = window.getComputedStyle(element);
			if (style.display === 'none' || style.visibility === 'hidden') return false;
			const rect = element.getBoundingClientRect();
			return rect.width > 0 && rect.height > 0;
		}

		return Array.from(document.querySelectorAll<HTMLElement>('button, input, select, textarea'))
			.filter((element) => {
				if (!isVisibleControl(element)) return false;
				if (element instanceof HTMLInputElement && element.type === 'hidden') return false;

				const text = element.innerText?.trim();
				const ariaLabel = element.getAttribute('aria-label')?.trim();
				const title = element.getAttribute('title')?.trim();
				return !(text || ariaLabel || title || hasAssociatedLabel(element));
			})
			.slice(0, 10)
			.map((element) => element.outerHTML.slice(0, 180));
	});

	expect(unnamedControls).toEqual([]);
}

test.describe('frontend a11y and responsive smoke', () => {
	test.beforeEach(async ({ page }) => {
		await installAuthenticatedSession(page);
		await installDeviceApiMocks(page);
	});

	const routes = [
		{ path: '/', readyText: 'CO' },
		{ path: '/system/matrix', readyText: 'Matrix LED Settings' },
		{ path: '/wifi/sta', readyText: 'WiFi' },
		{ path: '/settings/notifications/telegram', readyText: 'Telegram' },
		{ path: '/system/power', readyText: 'Power' }
	];

	for (const route of routes) {
		test(`${route.path} has named controls and no mobile overflow`, async ({ page }) => {
			for (const viewport of [
				{ width: 1366, height: 900 },
				{ width: 360, height: 740 }
			]) {
				await page.setViewportSize(viewport);
				await page.goto(route.path);
				await expect(page.getByText(route.readyText).first()).toBeVisible({ timeout: 30_000 });
				await expectInteractiveControlsHaveNames(page);
				await expectNoHorizontalOverflow(page);
			}
		});
	}
});
