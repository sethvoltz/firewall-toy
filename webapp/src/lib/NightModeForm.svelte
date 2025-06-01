<script lang="ts">
  import { onMount } from "svelte";

  // Night mode state
  let nightEnabled: boolean = false;
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
      nightEnabled = !!data.nightEnabled;
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
        nightEnabled,
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
      console.log("Night settings updated:", data);
    } catch (e: any) {
      nightApiError = e.message;
    } finally {
      nightApiLoading = false;
    }
  }

  onMount(() => {
    getNightSettings();
  });
</script>

<form class="mb-4 space-y-3" on:submit|preventDefault={setNightSettings}>
  <div class="flex flex-col gap-3 max-w-md">
    <label class="flex items-center gap-2">
      <input type="checkbox" bind:checked={nightEnabled} />
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
