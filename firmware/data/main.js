document.getElementById('echoBtn').addEventListener('click', async () => {
  const payload = { message: 'Hello from the browser!' };
  const res = await fetch('/api/echo', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  const text = await res.text();
  document.getElementById('result').textContent = text;
});
