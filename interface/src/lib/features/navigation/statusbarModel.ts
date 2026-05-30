import type { SystemStatus } from '$lib/types/system/systemStatus';
import type { MenuItem } from './menuConfig';

const VALID_EPOCH_MS = 1_000_000_000_000;

function pad2(n: number): string {
	return String(n).padStart(2, '0');
}

export function createStatusbarClock(
	status: Pick<SystemStatus, 'timestamp' | 'lastUpdate'>,
	nowMs: number
) {
	const serverTs = status.timestamp;

	const date =
		serverTs > VALID_EPOCH_MS
			? new Date(serverTs + Math.max(0, nowMs - status.lastUpdate))
			: new Date(nowMs);

	return {
		date: `${pad2(date.getDate())}.${pad2(date.getMonth() + 1)}.${date.getFullYear()}`,
		clock: `${pad2(date.getHours())}:${pad2(date.getMinutes())}`
	};
}

export function getStatusbarTempColor(tempCelsius: number) {
	const minTemp = 40;
	const maxTemp = 90;
	const startHue = 140;
	const endHue = 0;

	let hue = startHue - ((tempCelsius - minTemp) / (maxTemp - minTemp)) * (startHue - endHue);
	hue = Math.max(endHue, Math.min(startHue, hue));

	return `color: color-mix(in srgb, hsl(${hue}, 75%, 50%) 30%, currentColor)`;
}

export function resolveStatusbarTitle(
	menu: MenuItem[],
	pathname: string,
	fallbackTitle: string
): string {
	for (const item of menu) {
		if (item.href === pathname) return item.title;
		if (item.submenu) {
			const subItem = item.submenu.find((entry) => entry.href === pathname);
			if (subItem) return subItem.title;
		}
	}

	return fallbackTitle;
}
