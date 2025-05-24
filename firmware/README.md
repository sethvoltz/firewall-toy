# Firewall Toy

## Description

A cute desk toy in the shape of a firewall (a wall... on fire) that can display statuses through the color of the flame on top. The device connects to your network and exposes a CoAP endpoint for other devices to send those status messages and a browser-based HTTP interface for friendly config.


## Notes

Below are random notes for future reference and expansion into full documentation.

Upload the filesystem to the ESP32 using PlatformIO:

```bash
pio run --target uploadfs
```

Wipe the filesystem and wifi credentials:

```bash
pio run --target erase
```


## Kudos

This [favicon generator](https://realfavicongenerator.net) is indeed the real deal.


## Enhancements

### Firmware

- [ ] Allow resetting wifi or otherwise launching the captive portal from the web interface.
- [ ] Get internet time and allow timezone setting in the web UI
- [ ] Allow dimming or sleeping the display at night
- [ ] Host documentation on the web UI

### Hardware

- [ ] Add a button on the bottom to trigger actions, press and hold to enable captive portal
