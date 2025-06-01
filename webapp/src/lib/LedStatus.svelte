<script lang="ts">
  import { onMount } from 'svelte';

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
    connectWs();
    return cleanupWs;
  });
</script>

<div class="mb-4 flex flex-col gap-2 items-center">
  <div class="flex items-center gap-2">
    {#if liveLeds.length > 0}
      <div class="flex flex-wrap gap-1 rounded-xl border p-2 border-gray-100">
        {#each liveLeds as led, i}
          <span
            class="size-6 inline-block rounded-full transition-colors duration-500"
            style="background: rgb({led.r},{led.g},{led.b});"
            title={`LED ${i}: R${led.r} G${led.g} B${led.b}`}
          ></span>
        {/each}
      </div>
    {:else}
      <span class="text-gray-400">(waiting for LED state)</span>
    {/if}
  </div>

  <div class="flex items-center gap-2 mt-2">
    <!-- Status circle -->
    <span
      class={[
        "size-4 inline-block rounded-full transition-colors duration-300",
        wsConnected ? 'bg-green-500' : 'bg-gray-500',
        wsError ? 'bg-red-500' : ''
      ]}
      title={wsConnected ? 'Connected' : 'Disconnected'}
    ></span>
    <div class="relative w-36 h-5 rounded overflow-hidden border border-neutral-400 dark:border-neutral-600">
      <div
        class="h-full bg-neutral-500 transition-all duration-200"
        style="width: {liveBrightness !== null ? (liveBrightness / 255) * 100 : 0}%;"
      ></div>
    </div>
    {#if liveBrightness !== null}
      <span class="text-sm">{liveBrightness}</span>
    {:else}
      <span class="text-gray-500">(no data)</span>
    {/if}
  </div>

  <!-- Remove old status text -->
  <div class="flex items-center gap-3">
    {#if wsError}
      <span class="text-red-500 text-xs ml-2">{wsError}</span>
    {/if}
  </div>
</div>
