<script lang="ts">
  import { onMount } from 'svelte';

  let ws: WebSocket | null = null;
  let wsConnected: boolean = false;
  let wsError: string = "";
  let liveBrightness: number | null = null;
  let liveLeds: { r: number; g: number; b: number }[] = [];

  const HEARTBEAT_TIMEOUT = 1000;
  const CONNECTION_TIMEOUT = 1000;
  const RECONNECT_DELAY_MAX = 8000;

  let lastMessageTime: number = 0;
  let heartbeatInterval: ReturnType<typeof setInterval> | null = null;
  let reconnectDelay = 500;

  function cleanupWs() {
    if (heartbeatInterval) {
      clearInterval(heartbeatInterval);
      heartbeatInterval = null;
    }
    if (ws) {
      ws.onopen = null;
      ws.onclose = null;
      ws.onerror = null;
      ws.onmessage = null;
      ws.close();
      ws = null;
    }
  }

  function connectWs() {
    wsError = "";
    cleanupWs(); // Always clean up before opening a new one

    let connectionTimeout: ReturnType<typeof setTimeout> | null = null;
    try {
      ws = new WebSocket(`ws://${window.location.host}/ws`);

      // Manual connection timeout (e.g. 1s)
      connectionTimeout = setTimeout(() => {
        if (ws && ws.readyState === WebSocket.CONNECTING) {
          wsError = "WebSocket connection timed out";
          ws.close(); // This will trigger onclose and reconnect
        }
      }, CONNECTION_TIMEOUT);

      ws.onopen = () => {
        if (connectionTimeout) {
          clearTimeout(connectionTimeout);
          connectionTimeout = null;
        }
        wsConnected = true;
        wsError = "";
        lastMessageTime = Date.now();
        reconnectDelay = 500; // Reset delay on successful connect
        if (!heartbeatInterval) {
          heartbeatInterval = setInterval(() => {
            if (wsConnected && Date.now() - lastMessageTime > HEARTBEAT_TIMEOUT) {
              wsError = "Connection lost (timeout)";
              wsConnected = false;
              ws?.close(); // This will trigger onclose and reconnect
            }
          }, HEARTBEAT_TIMEOUT / 2); // Nyquist-ish
        }
      };

      ws.onclose = () => {
        if (connectionTimeout) {
          clearTimeout(connectionTimeout);
          connectionTimeout = null;
        }
        wsConnected = false;
        ws = null;
        if (heartbeatInterval) {
          clearInterval(heartbeatInterval);
          heartbeatInterval = null;
        }
        console.log(`WebSocket closed, attempting to reconnect in ${reconnectDelay}ms...`);
        setTimeout(connectWs, reconnectDelay);
        reconnectDelay = Math.min(reconnectDelay * 2, RECONNECT_DELAY_MAX);
      };

      ws.onerror = (e) => {
        if (connectionTimeout) {
          clearTimeout(connectionTimeout);
          connectionTimeout = null;
        }
        wsError = "WebSocket error";
        wsConnected = false;
      };

      ws.onmessage = (event) => {
        lastMessageTime = Date.now();
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
      if (connectionTimeout) {
        clearTimeout(connectionTimeout);
        connectionTimeout = null;
      }
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
