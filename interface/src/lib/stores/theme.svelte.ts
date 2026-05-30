import { browser } from '$app/environment';

interface ThemeSettings {
	theme: string;
	borderRadius: string; // e.g., '0.5rem'
	primaryColor: string; // e.g., '#3b82f6'
	animationSpeed: string; // e.g., '0.5s'
	fontSize?: string; // e.g. '100%'
	borderWidth?: string; // e.g. '1px'
	focusScale?: string; // e.g. '0.95'
}

const DEFAULT_SETTINGS: ThemeSettings = {
	theme: 'business',
	borderRadius: '0.25rem', // Default for business
	primaryColor: '', // Empty means use theme default
	animationSpeed: '0.2s',
	fontSize: '100%',
	borderWidth: '1px',
	focusScale: '0.95'
};

class ThemeStore {
	settings = $state<ThemeSettings>(DEFAULT_SETTINGS);
	private skipInitialEffect = true;

	constructor() {
		if (browser) {
			const stored = localStorage.getItem('theme_settings');
			if (stored) {
				try {
					this.settings = { ...DEFAULT_SETTINGS, ...JSON.parse(stored) };
				} catch {
					// Invalid JSON, use defaults
				}
			}
			const initialSettings = $state.snapshot(this.settings);
			this.persist(initialSettings);
			this.apply(initialSettings);
			console.info('[ThemeStore] Loaded and applied settings:', initialSettings);
		}

		$effect.root(() => {
			$effect(() => {
				const settings = $state.snapshot(this.settings);
				if (this.skipInitialEffect) {
					this.skipInitialEffect = false;
					return;
				}

				this.persist(settings);
				this.apply(settings);
			});
		});
	}

	persist(settings: ThemeSettings = this.settings) {
		if (browser) {
			localStorage.setItem('theme_settings', JSON.stringify(settings));
		}
	}

	apply(settings: ThemeSettings = this.settings) {
		if (!browser) return;
		const html = document.documentElement;
		const root = document.documentElement;

		// Apply Theme
		html.setAttribute('data-theme', settings.theme);

		// Layout / Typography
		html.style.fontSize = settings.fontSize || '100%';

		// Roundness
		// DaisyUI v5 uses --radius-* variables
		const r = settings.borderRadius;
		root.style.setProperty('--radius-box', r);
		root.style.setProperty('--radius-btn', r);
		root.style.setProperty('--radius-field', r);
		root.style.setProperty('--radius-selector', r);

		// Tailwind v4 global radius
		root.style.setProperty('--radius', r);

		// Animation
		root.style.setProperty('--animation-btn', settings.animationSpeed);
		root.style.setProperty('--animation-input', settings.animationSpeed);

		// Borders & Effects
		root.style.setProperty('--border-btn', settings.borderWidth || '1px');
		root.style.setProperty('--btn-focus-scale', settings.focusScale || '0.95');
	}

	setTheme(theme: string) {
		this.settings.theme = theme;
	}

	setRadius(radius: string) {
		this.settings.borderRadius = radius;
	}

	setAnimationSpeed(speed: string) {
		this.settings.animationSpeed = speed;
	}

	setFontSize(size: string) {
		this.settings.fontSize = size;
	}

	setBorderWidth(width: string) {
		this.settings.borderWidth = width;
	}

	setFocusScale(scale: string) {
		this.settings.focusScale = scale;
	}
}

export const themeStore = new ThemeStore();
