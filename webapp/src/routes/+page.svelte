<script lang="ts">
  type EchoResult = {
    timestamp: string;
    data: any;
  };

  let results: EchoResult[] = [];
  let echoText: string = "Hello, Firewall Toy!";

  function sendEcho() {
    fetch('/api/echo', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ message: echoText })
    })
      .then(response => response.json())
      .then(data => {
        results = [
          ...results,
          {
            timestamp: new Date().toLocaleString(),
            data
          }
        ];
      })
      .catch(error => {
        results = [
          ...results,
          {
            timestamp: new Date().toLocaleString(),
            data: { error: error.message }
          }
        ];
      });
  }

  function clearResults() {
    results = [];
  }
</script>

<svelte:head>
  <title>Firewall Toy Control</title>
</svelte:head>

<h1 class="text-xl">Firewall Toy Control</h1>

<form on:submit|preventDefault={sendEcho} style="margin-bottom: 1em;">
  <input
    type="text"
    bind:value={echoText}
    placeholder="Enter echo text"
    style="width: 300px; margin-right: 0.5em;"
  />
  <button type="submit">Send Echo API</button>
  <button type="button" on:click={clearResults} style="margin-left: 0.5em;">Clear Results</button>
</form>

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
