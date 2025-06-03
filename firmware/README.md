# Firewall Toy Firmware

## Features

This firmware provides the following features:

- Captive portal for easy WiFi setup
- Web interface for controlling the device
- mDNS support for easy discovery


## Setup & Installation

1. Install [PlatformIO](https://platformio.org/install/cli) if you haven't already.
2. Clone this repository.
3. Ensure it builds and has dependencies installed by running `pio run`
4. Upload the filesystem to the ESP32 by running `pio run --target uploadfs`. This will copy the `data` directory contents to the ESP32's filesystem.
4. Once build is successful, plug in the ESP32 and run `pio run --target upload` to upload the firmware to your ESP32 device.

**Note:** If the device ever needs a full hard reset, you can run `pio run --target erase` to wipe the ESP32's flash memory, including the filesystem and WiFi credentials.


## Usage

Once the firmware is uploaded, the device will start in access point mode. Connect to the WiFi network named `FirewallToy` and open a web browser to `http://<device-ip>` or `http://firewalltoy.local` to access the web interface. Generally, however, your machine will detect that it is a captive portal and redirect you to the setup page automatically. Once you set up the WiFi credentials, the device will reboot and connect to your WiFi network, showing the default animation on the flame display. To communicate with the device, you can either use the web interface or direct API calls.


### Web Interface

Access the web interface by navigating to `http://<device-ip>` or `http://firewalltoy.local` in your web browser. The web interface allows you to control the device's mode and color settings. You can change the mode to "flame", "rainbow", or "off", and set the color using RGB values.


## Kudos

This [favicon generator](https://realfavicongenerator.net) is indeed the real deal.


## Enhancements

### Firmware

- [ ] Allow resetting wifi or otherwise launching the captive portal from the web interface.
- [ ] Get internet time and allow timezone setting in the web UI
- [ ] Allow dimming or sleeping the display at night
- [ ] Host documentation on the web UI
