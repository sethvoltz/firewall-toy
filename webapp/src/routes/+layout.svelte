<svelte:options runes={true} />
<script lang="ts">
	import '../app.css';
	import { page } from '$app/state';

	import LedStatus from '$lib/LedStatus.svelte';

	let { children } = $props();
</script>

<svelte:head>
  <title>Firewall Toy Control</title>
</svelte:head>

<div class="flex min-h-screen bg-neutral-50 dark:bg-neutral-900">
	<aside class="w-64 bg-white dark:bg-neutral-800 border-r border-neutral-200 dark:border-neutral-700 flex flex-col items-center p-4">
		<LedStatus />
		<nav class="flex flex-col gap-4 w-full">
			<a href="/"
				class={[
					"px-4 py-2 rounded text-neutral-800 dark:text-neutral-100 hover:bg-neutral-100 dark:hover:bg-neutral-700 font-medium transition",
					page.url.pathname === '/' ? 'bg-neutral-200 dark:bg-neutral-700' : ''
				]}>
				Color and Mode
			</a>
			<a href="/nightMode"
				class={[
					"px-4 py-2 rounded text-neutral-800 dark:text-neutral-100 hover:bg-neutral-100 dark:hover:bg-neutral-700 font-medium transition",
					page.url.pathname === '/nightMode' ? 'bg-neutral-200 dark:bg-neutral-700' : ''
				]}>
				Night Mode
			</a>
		</nav>
	</aside>

	<main class="flex-1 flex flex-col min-h-screen">
		{@render children()}
	</main>
</div>
