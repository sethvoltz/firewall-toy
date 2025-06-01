<script lang="ts">
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

  let mode: 'flame' | 'static' = 'flame';
  let color = { r: 255, g: 100, b: 255 };

  async function sendMode() {
    try {
      const response = await fetch('/api/mode', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode, color })
      });
      const data = await response.json();
      console.log('Mode response:', data);
    } catch (error: any) {
      console.error('Error sending mode:', error);
    }
  }

  function selectColor(c: { r: number; g: number; b: number }) {
    color = c
  }
</script>

<form id="color-mode-form" on:submit={sendMode} class="mb-4 space-y-3">
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
