import type { LayoutLoad } from './$types';

// This can be false if you're using a fallback (i.e. SPA mode)
export const prerender = false;
export const ssr = false;

export const load = (async () => {
	return {
		title: 'MatrixHub',
		github: 'theelims/ESP32-sveltekit',
		copyright: '2025 theelims',
		appName: 'MatrixHub'
	};
}) satisfies LayoutLoad;
