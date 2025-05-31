import tailwindcss from '@tailwindcss/vite';
import { svelteTesting } from '@testing-library/svelte/vite';
import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig, loadEnv } from 'vite';

export default defineConfig(({ mode }) => {
	const env = loadEnv(mode, process.cwd(), '');
	const apiIp = env.VITE_FIREWALLTOY_API_IP || 'firewalltoy.local';
	console.log(`[vite.config] Using API endpoint: http://${apiIp}`);

	return {
		plugins: [tailwindcss(), sveltekit()],
		build: {
			rollupOptions: {
				output: {
					manualChunks: () => 'main.js', // bundle all into one file
				}
			},
			cssCodeSplit: false // bundle all CSS into JS
		},
		server: {
			proxy: {
				'/api': { target: `http://${apiIp}`, changeOrigin: true },
				'/ws': { target: `http://${apiIp}`, changeOrigin: true, ws: true }
			}
		},
		test: {
			workspace: [
				{
					extends: './vite.config.ts',
					plugins: [svelteTesting()],
					test: {
						name: 'client',
						environment: 'jsdom',
						clearMocks: true,
						include: ['src/**/*.svelte.{test,spec}.{js,ts}'],
						exclude: ['src/lib/server/**'],
						setupFiles: ['./vitest-setup-client.ts']
					}
				},
				{
					extends: './vite.config.ts',
					test: {
						name: 'server',
						environment: 'node',
						include: ['src/**/*.{test,spec}.{js,ts}'],
						exclude: ['src/**/*.svelte.{test,spec}.{js,ts}']
					}
				}
			]
		}
	};
});
