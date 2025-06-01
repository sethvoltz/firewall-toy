<script lang="ts">
  import NightModeForm from '$lib/NightModeForm.svelte';
</script>

<header class="p-4 border-b border-neutral-200 dark:border-neutral-700 bg-white/80 dark:bg-neutral-900/80 backdrop-blur">
  <h1 class="text-2xl font-bold text-neutral-900 dark:text-neutral-100">
    Night Mode
  </h1>
</header>

<section class="p-4 space-y-6">
  <NightModeForm />
</section>
