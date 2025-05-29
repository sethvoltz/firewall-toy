# Firewall Toy Webapp

This is the web application component of the firewall toy project, built using [SvelteKit](https://kit.svelte.dev/). It provides the embedded web UI for the toy and builds into the firmware `data/` directory.


## Developing

Run the following to set up:

```bash
npm install
echo "VITE_FIREWALLTOY_API_IP=<IP_OF_RUNNING_HARDWARE>" >>.env
npm run dev
```

This will install the necessary dependencies and start the development server. It also proxies all requests to `/api` to the specified IP address of the running hardware, allowing you to interact with the firewall toy's API without needing to run a production build and upload.


## Building

To build a production application, run:

```bash
npm run build
```

This will build the app and dump it into the firmware `data/` directory, which can then be uploaded to the firewall toy's firmware. See the [firmware readme](../firmware/README.md) for instructions on how to upload the built files. You can preview the production build with `npm run preview`.
