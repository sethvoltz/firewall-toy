<script lang="ts">
  import { onMount } from 'svelte';

  type APIResult = {
    timestamp: string;
    data: any;
  };

  let results: APIResult[] = [];
  
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

  // Night mode state
  let nightModeEnabled: boolean = false;
  let nightStartLocal: string = "22:00"; // local time string, e.g. "22:00"
  let nightEndLocal: string = "07:00";
  let dayBrightness: number = 255;
  let nightBrightness: number = 32;
  let nightApiLoading: boolean = false;
  let nightApiError: string = "";

  // Convert local time string ("HH:MM") to UTC hour (int)
  function localTimeToUtcHour(localTime: string): number {
    const [h, m] = localTime.split(":").map(Number);
    const d = new Date();
    d.setHours(h, m, 0, 0);
    return (d.getUTCHours());
  }

  // Convert UTC hour (int) to local time string ("HH:MM")
  function utcHourToLocalTime(utcHour: number): string {
    const d = new Date();
    d.setUTCHours(utcHour, 0, 0, 0);
    return d.getHours().toString().padStart(2, "0") + ":00";
  }

  async function getNightSettings() {
    nightApiLoading = true;
    nightApiError = "";
    try {
      const res = await fetch("/api/brightness", { method: "GET" });
      const data = await res.json();
      nightModeEnabled = !!data.nightModeEnabled;
      nightStartLocal = utcHourToLocalTime(data.nightStartHour);
      nightEndLocal = utcHourToLocalTime(data.nightEndHour);
      dayBrightness = data.dayBrightness;
      nightBrightness = data.nightBrightness;
    } catch (e: any) {
      nightApiError = e.message;
    } finally {
      nightApiLoading = false;
    }
  }

  async function setNightSettings() {
    nightApiLoading = true;
    nightApiError = "";
    try {
      const body = {
        nightModeEnabled,
        nightStartHour: localTimeToUtcHour(nightStartLocal),
        nightEndHour: localTimeToUtcHour(nightEndLocal),
        dayBrightness,
        nightBrightness
      };
      const res = await fetch("/api/brightness", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body)
      });
      const data = await res.json();
      results = [
        ...results,
        { timestamp: new Date().toLocaleString(), data: { brightness: data } }
      ];
    } catch (e: any) {
      nightApiError = e.message;
    } finally {
      nightApiLoading = false;
    }
  }

  // WebSocket state for live LED data
  let ws: WebSocket | null = null;
  let wsConnected: boolean = false;
  let wsError: string = "";
  let liveBrightness: number | null = null;
  let liveLeds: { r: number; g: number; b: number }[] = [];

  function cleanupWs() {
    if (ws) {
      ws.onopen = null;
      ws.onclose = null;
      ws.onerror = null;
      ws.onmessage = null;
      ws?.close();
      ws = null;
    }
  }

  function connectWs() {
    wsError = "";

    cleanupWs(); // Always clean up before opening a new one

    try {
      ws = new WebSocket(`ws://${window.location.host}/ws`);
      ws.onopen = () => {
        wsConnected = true;
        wsError = "";
      };
      ws.onclose = () => {
        wsConnected = false;
        ws = null;
        setTimeout(connectWs, 2000);
      };
      ws.onerror = (e) => {
        wsError = "WebSocket error";
        wsConnected = false;
      };
      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          if (typeof data.brightness === "number" && Array.isArray(data.leds)) {
            liveBrightness = data.brightness;
            liveLeds = data.leds;
          }
        } catch (e) {
          wsError = "Invalid data from WebSocket";
        }
      };
    } catch (e: any) {
      wsError = e.message;
    }
  }

  onMount(() => {
    getNightSettings();
    connectWs();
    return cleanupWs;
  });
</script>

<svelte:head>
  <title>Firewall Toy Control</title>
</svelte:head>

<h1 class="text-xl font-bold mb-4">Firewall Toy Control</h1>

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

<!-- Night Mode Settings -->
<h2 class="text-lg font-semibold mt-8 mb-3">Night Mode Settings</h2>
<form class="mb-4 space-y-3" on:submit|preventDefault={setNightSettings}>
  <div class="flex flex-col gap-3 max-w-md">
    <label class="flex items-center gap-2">
      <input type="checkbox" bind:checked={nightModeEnabled} />
      Enable Night Mode
    </label>
    <div class="flex gap-4">
      <label class="flex flex-col text-sm">Night Start
        <input type="time" bind:value={nightStartLocal} class="input input-bordered w-28" />
      </label>
      <label class="flex flex-col text-sm">Night End
        <input type="time" bind:value={nightEndLocal} class="input input-bordered w-28" />
      </label>
    </div>
    <div class="flex gap-4">
      <label class="flex flex-col text-sm">Day Brightness
        <input type="number" min="0" max="255" bind:value={dayBrightness} class="input input-bordered w-20" />
      </label>
      <label class="flex flex-col text-sm">Night Brightness
        <input type="number" min="0" max="255" bind:value={nightBrightness} class="input input-bordered w-20" />
      </label>
    </div>
    <div class="flex gap-2 mt-2">
      <button type="button" class="btn" on:click={getNightSettings} disabled={nightApiLoading}>Get</button>
      <button type="submit" class="btn btn-primary" disabled={nightApiLoading}>Set</button>
      {#if nightApiLoading}
        <span class="ml-2 text-xs text-gray-500">Loading...</span>
      {/if}
    </div>
    {#if nightApiError}
      <div class="text-red-500 text-xs">{nightApiError}</div>
    {/if}
  </div>
</form>

<!-- Live LED State -->
<h2 class="text-lg font-semibold mt-8 mb-3">Live LED State</h2>
<div class="mb-4 flex flex-col gap-2">
  <div class="flex items-center gap-3">
    {#if wsConnected}
      <span class="text-green-600">Connected</span>
    {:else}
      <span class="text-red-600">Disconnected</span>
    {/if}
    {#if wsError}
      <span class="text-red-500 text-xs ml-2">{wsError}</span>
    {/if}
  </div>

  <div class="flex items-center gap-4 mt-2">
    <span class="font-medium">Brightness:</span>
    <div class="relative w-48 h-5 rounded overflow-hidden border border-neutral-400 dark:border-neutral-600">
      <div
        class="h-full bg-gradient-to-r from-green-950 to-green-100 transition-all duration-200"
        style="width: {liveBrightness !== null ? (liveBrightness / 255) * 100 : 0}%;"
      ></div>
    </div>
    {#if liveBrightness !== null}
      <span class="ml-2 font-mono">{liveBrightness}</span>
    {:else}
      <span class="ml-2 text-gray-500">(no data)</span>
    {/if}
  </div>

  <div class="flex items-center gap-2 mt-2">
    <span class="font-medium">LEDs:</span>
    {#if liveLeds.length > 0}
      <div class="flex flex-wrap gap-1 rounded-xl border p-2 border-gray-100">
        {#each liveLeds as led, i}
          <span
            class="inline-block rounded-full transition-colors duration-500"
            style="width: 24px; height: 24px; background: rgb({led.r},{led.g},{led.b});"
            title={`LED ${i}: R${led.r} G${led.g} B${led.b}`}
          ></span>
        {/each}
      </div>
    {:else}
      <span class="text-gray-400">(no data)</span>
    {/if}
  </div>
</div>

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
