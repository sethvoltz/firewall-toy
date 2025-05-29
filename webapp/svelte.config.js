import adapter from '@sveltejs/adapter-static';
import { vitePreprocess } from '@sveltejs/vite-plugin-svelte';

const config = {
	preprocess: vitePreprocess(),
	kit: {
		adapter: adapter({
			pages: '../firmware/data',
			assets: '../firmware/data',
			fallback: 'index.html',
			precompress: false
		}),
		paths: {
			base: '',
		}
	}
};

export default config;
