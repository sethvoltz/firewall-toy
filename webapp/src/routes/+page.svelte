<script lang="ts">
  type EchoResult = {
    timestamp: string;
    data: any;
  };

  let results: EchoResult[] = [];
  let echoText: string = "Hello, Firewall Toy!";

  let mode: 'flame' | 'static' = 'flame';
  let color = { r: 255, g: 100, b: 255 };

  const colorGrid = [
    { r: 255, g: 0, b: 0 },    // Red
    { r: 255, g: 128, b: 0 },  // Orange
    { r: 255, g: 255, b: 0 },  // Yellow
    { r: 0, g: 255, b: 0 },    // Green
    { r: 0, g: 255, b: 255 },  // Cyan
    { r: 0, g: 0, b: 255 },    // Blue
    { r: 255, g: 0, b: 255 },  // Magenta
    { r: 255, g: 255, b: 255 },// White
    { r: 0, g: 0, b: 0 },      // Black
  ];

  function selectColor(c: { r: number; g: number; b: number }) {
    color = { ...c };
  }

  async function sendEcho() {
    try {
      const response = await fetch('/api/echo', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ message: echoText })
      });
      const data = await response.json();
      results = [
        ...results,
        {
          timestamp: new Date().toLocaleString(),
          data
        }
      ];
    } catch (error: any) {
      results = [
        ...results,
        {
          timestamp: new Date().toLocaleString(),
          data: { error: error.message }
        }
      ];
    }
  }

  async function sendMode() {
    try {
      const response = await fetch('/api/mode', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode, color })
      });
      const data = await response.json();
      results = [
        ...results,
        {
          timestamp: new Date().toLocaleString(),
          data: { modeResponse: data }
        }
      ];
    } catch (error: any) {
      results = [
        ...results,
        {
          timestamp: new Date().toLocaleString(),
          data: { error: error.message }
        }
      ];
    }
  }

  function clearResults() {
    results = [];
  }
</script>

<svelte:head>
  <title>Firewall Toy Control</title>
</svelte:head>

<h1 class="text-xl font-bold mb-4">Firewall Toy Control</h1>

<h2 class="text-lg font-semibold mb-3">Send Echo</h2>
<form on:submit|preventDefault={sendEcho} class="mb-4 flex flex-wrap items-center gap-2">
  <input
    type="text"
    bind:value={echoText}
    placeholder="Enter echo text"
    class="input input-bordered w-72 mr-2 px-2 py-1 rounded border border-gray-300 focus:outline-none focus:ring focus:border-blue-400"
  />
  <button type="submit" class="btn btn-primary">Send Echo</button>
</form>

<h2 class="text-lg font-semibold mt-8 mb-3">Set Mode & Color</h2>

<form on:submit|preventDefault={sendMode} class="mb-4 space-y-3">
  <div class="flex flex-row gap-6 items-start">
    <div class="flex flex-col gap-3">
      <div class="flex flex-col gap-2">
        <label class="inline-flex items-center cursor-pointer">
          <input type="radio" bind:group={mode} value="flame" class="form-radio text-pink-500" />
          <span class="ml-2">Flame</span>
        </label>
        <label class="inline-flex items-center cursor-pointer">
          <input type="radio" bind:group={mode} value="static" class="form-radio text-blue-500" />
          <span class="ml-2">Static</span>
        </label>
      </div>
      <div class="flex items-center gap-4 mt-2">
        <label class="flex items-center">R:
          <input type="number" min="0" max="255" bind:value={color.r} class="input input-bordered w-16 ml-1 px-1 py-0.5 rounded border border-gray-300" />
        </label>
        <label class="flex items-center">G:
          <input type="number" min="0" max="255" bind:value={color.g} class="input input-bordered w-16 ml-1 px-1 py-0.5 rounded border border-gray-300" />
        </label>
        <label class="flex items-center">B:
          <input type="number" min="0" max="255" bind:value={color.b} class="input input-bordered w-16 ml-1 px-1 py-0.5 rounded border border-gray-300" />
        </label>
      </div>
    </div>
    <div class="grid grid-cols-3 gap-2 w-28 ml-4">
      {#each colorGrid as c}
        <button
          type="button"
          on:click={() => selectColor(c)}
          class="w-8 h-8 rounded border-2 border-gray-300 focus:outline-none focus:ring-2 focus:ring-blue-400"
          style="background: rgb({c.r},{c.g},{c.b});"
          title={`R:${c.r} G:${c.g} B:${c.b}`}
          aria-label={`Select color R:${c.r} G:${c.g} B:${c.b}`}
        ></button>
      {/each}
    </div>
  </div>
  <button type="submit" class="btn btn-primary">Send Mode</button>
</form>

<button type="button" on:click={clearResults} class="btn mb-2">Clear Results</button>
<ul class="border p-4 border-dashed rounded">
  {#if results.length === 0}
    <li>No results yet. Send an echo request!</li>
  {/if}
  {#each results as result (result.timestamp)}
    <li>
      <strong>{result.timestamp}:</strong>
      <pre>{JSON.stringify(result.data, null, 2)}</pre>
    </li>
  {/each}
</ul>
