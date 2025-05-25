# Firewall Toy Firmware

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

Make CoAP requests to the device using `coap-client`:

```bash
brew install libcoap
coap-client -m put -e '{"mode":"flame","color":{"r":255,"g":0,"b":255}}' coap://<ip>/mode
```

Note that while you can use the mDNS name in the `coap://` URL, there seems to be an issue with resolution within `coap-client` that makes it slow (~5s per request). IP addresses return instantly.


## Kudos

This [favicon generator](https://realfavicongenerator.net) is indeed the real deal.


## Enhancements

### Firmware

- [ ] Allow resetting wifi or otherwise launching the captive portal from the web interface.
- [ ] Get internet time and allow timezone setting in the web UI
- [ ] Allow dimming or sleeping the display at night
- [ ] Host documentation on the web UI
