#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <coap-simple.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>

// =---------------------------------------------------------------------------------= Settings =--=

#define LED_PIN                     D1
#define NUM_LEDS                    3
#define BRIGHTNESS                  128
#define ANIMATION_MS                33
#define SETTINGS_PATH               "/settings.json"
#define FLAME_BLEND_STEPS           8


// =----------------------------------------------------------------------------------= Globals =--=

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiUDP udp;
Coap coap(udp);

// Animation Mode
enum AnimationMode {
  ANIMATION_STATIC,
  ANIMATION_FLAME
};

AnimationMode currentMode = ANIMATION_FLAME;
uint8_t currentR = 255, currentG = 110, currentB = 15;

// System Mode
enum StatusMode {
  STATUS_BOOT,
  STATUS_WIFI_CONNECTING,
  STATUS_PORTAL,
  STATUS_READY
};

StatusMode statusMode = STATUS_BOOT;
unsigned long statusBlinkLast = 0;
bool statusBlinkState = false;

struct Settings {
  String mdnsName = "firewalltoy";
};

Settings settings;

// Flame animation state
struct HSV {
  float h, s, v;
};

HSV currentColors[NUM_LEDS];
HSV targetColors[NUM_LEDS];
uint8_t flameStep = 0;


// =-------------------------------------------------------------------------------= Signatures =--=

// TODO: Move to header file
void animationSetup();
void animationLoop();
void animationFrame();
void coapSetup();
void coapLoop();
void wifiSetup();
void mdnsSetup();
void loadSettings();
void saveSettings();
void handlePutMode(CoapPacket &packet, IPAddress ip, int port);
void setStatusColor(uint8_t r, uint8_t g, uint8_t b);
HSV lerpHSV(const HSV& c1, const HSV& c2, float t);
HSV flameColor(const HSV& base, float h_jitter, float s_jitter, float v_jitter);
HSV rgbToHsv(uint8_t r, uint8_t g, uint8_t b);

// =--------------------------------------------------------------------------------= Functions =--=

// Helper: Linear interpolation between two HSV colors, handling hue wraparound
HSV lerpHSV(const HSV& c1, const HSV& c2, float t) {
  HSV out;
  float dh = c2.h - c1.h;
  if (fabs(dh) > 0.5f) {
    if (dh > 0) out.h = fmodf(c1.h + 1.0f + (c2.h - (c1.h + 1.0f)) * t, 1.0f);
    else out.h = fmodf(c1.h - 1.0f + (c2.h - (c1.h - 1.0f)) * t, 1.0f);
  } else {
    out.h = fmodf(c1.h + dh * t, 1.0f);
  }
  out.s = c1.s + (c2.s - c1.s) * t;
  out.v = c1.v + (c2.v - c1.v) * t;
  return out;
}

// Helper: Generate a random float between a and b, similar to Python's random.uniform(a, b)
float randomFloat(float a, float b) {
  return a + ((b - a) * random(0, 1001) / 1000.0);
}

// Helper: Jitter a base HSV color for flame effect
HSV flameColor(const HSV& base, float h_jitter = 0.01f, float s_jitter = 0.05f, float v_jitter = 0.4f) {
  HSV out = base;
  out.h = fmodf(base.h + randomFloat(-h_jitter, h_jitter), 1.0f);
  out.s = fminf(1.0f, fmaxf(0.0f, base.s + randomFloat(-s_jitter, s_jitter)));
  out.v = fminf(1.0f, fmaxf(0.0f, base.v + randomFloat(-v_jitter, v_jitter)));
  return out;
}

// Helper: Convert RGB (0-255) to HSV (0-1)
HSV rgbToHsv(uint8_t r, uint8_t g, uint8_t b) {
  float fr = r / 255.0f, fg = g / 255.0f, fb = b / 255.0f;
  float mx = fmaxf(fr, fmaxf(fg, fb)), mn = fminf(fr, fminf(fg, fb));
  float h, s, v = mx;
  float d = mx - mn;
  s = mx == 0 ? 0 : d / mx;
  if (d == 0) h = 0;
  else if (mx == fr) h = fmodf((fg - fb) / d, 6.0f) / 6.0f;
  else if (mx == fg) h = ((fb - fr) / d + 2.0f) / 6.0f;
  else h = ((fr - fg) / d + 4.0f) / 6.0f;
  if (h < 0) h += 1.0f;
  return {h, s, v};
}

// Animation
void animationSetup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show(); // Initialize all pixels to 'off'

  // Initialize flame animation state
  HSV base = rgbToHsv(currentR, currentG, currentB);
  for (int i = 0; i < NUM_LEDS; i++) {
    currentColors[i] = base;
    targetColors[i] = flameColor(base);
  }
  flameStep = 0;
}

void animationLoop() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();

  if (now - lastUpdate >= ANIMATION_MS) {
    lastUpdate = now;

    // Status LED logic
    switch (statusMode) {
      case STATUS_BOOT:
        setStatusColor(0, 255, 255); // Cyan
        break;

      case STATUS_WIFI_CONNECTING:
        setStatusColor(0, 0, 255); // Blue
        break;

      case STATUS_PORTAL:
        if (now - statusBlinkLast > 500) {
          statusBlinkLast = now;
          statusBlinkState = !statusBlinkState;
          if (statusBlinkState) setStatusColor(0, 0, 255); // Blue
          else setStatusColor(0, 0, 0); // Off
        }
        break;

      case STATUS_READY:
        animationFrame();
        break;
    }
  }
}

void animationFrame() {
  if (currentMode == ANIMATION_STATIC) {
    uint32_t color = strip.Color(currentR, currentG, currentB);
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, color);
    }
    strip.show();
  } else if (currentMode == ANIMATION_FLAME) {
    float t = (float)flameStep / (float)FLAME_BLEND_STEPS;
    for (int i = 0; i < NUM_LEDS; i++) {
      HSV blended = lerpHSV(currentColors[i], targetColors[i], t);
      uint32_t color = strip.ColorHSV((uint16_t)(blended.h * 65535.0f), (uint8_t)(blended.s * 255.0f), (uint8_t)(blended.v * 255.0f));
      strip.setPixelColor(i, color);
    }
    strip.show();
    flameStep++;
    if (flameStep > FLAME_BLEND_STEPS) {
      flameStep = 0;
      for (int i = 0; i < NUM_LEDS; i++) {
        currentColors[i] = targetColors[i];
        HSV base = rgbToHsv(currentR, currentG, currentB);
        targetColors[i] = flameColor(base);
      }
    }
  }
}

void setStatusColor(uint8_t r, uint8_t g, uint8_t b) {
  strip.fill(strip.Color(r, g, b));
  strip.show();
}


// CoAP
void coapSetup() {
  Serial.println("[CoAP] Starting UDP and registering /mode handler");
  udp.begin(5683);
  coap.server(handlePutMode, "mode");
  Serial.println("[CoAP] CoAP server ready");
}

void coapLoop() {
  coap.loop();
}

void handlePutMode(CoapPacket &packet, IPAddress ip, int port) {
  Serial.print("[CoAP] PUT /mode from ");
  Serial.print(ip);
  Serial.print(":");
  Serial.println(port);
  Serial.print("[CoAP] Payload: ");
  for (size_t i = 0; i < packet.payloadlen; i++) Serial.print((char)packet.payload[i]);
  Serial.println();

  // Parse JSON payload
  StaticJsonDocument<192> doc;
  DeserializationError err = deserializeJson(doc, packet.payload, packet.payloadlen);

  if (err) {
    Serial.println("[CoAP] Invalid JSON received");
    coap.sendResponse(ip, port, packet.messageid, "Invalid JSON");
    return;
  }

  // Expecting: {"mode": "static"|"flame", "color": {"r":0-255, "g":0-255, "b":0-255}}
  const char* mode = doc["mode"] | "static";
  if (strcmp(mode, "static") == 0) {
    currentMode = ANIMATION_STATIC;
  } else if (strcmp(mode, "flame") == 0) {
    currentMode = ANIMATION_FLAME;
  }
  Serial.print("[CoAP] Mode: ");
  Serial.println(mode);

  // Only update color if present
  if (doc.containsKey("color")) {
    JsonObject color = doc["color"].as<JsonObject>();
    if (color.containsKey("r")) currentR = color["r"];
    if (color.containsKey("g")) currentG = color["g"];
    if (color.containsKey("b")) currentB = color["b"];
  }

  Serial.printf("[CoAP] Color: r=%d, g=%d, b=%d\n", currentR, currentG, currentB);
  coap.sendResponse(ip, port, packet.messageid, "OK");
  Serial.println("[CoAP] LED color updated and response sent");
}


// WIFI & mDNS
void wifiSetup() {
  statusMode = STATUS_WIFI_CONNECTING;

  Serial.println("[WiFi] Starting WiFiManager for captive portal if needed...");
  WiFiManager wifiManager;
  WiFiManagerParameter mdnsParam("mdns", "mDNS Name", settings.mdnsName.c_str(), 32);
  wifiManager.addParameter(&mdnsParam);

  String apName = "FirewallToy-Setup-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  wifiManager.setConfigPortalTimeout(180); // 3 min timeout
  wifiManager.setEnableConfigPortal(true); // Always allow config portal

  // Set status to portal mode if config portal is started
  wifiManager.setAPCallback([](WiFiManager*) {
    statusMode = STATUS_PORTAL;
  });
  wifiManager.autoConnect(apName.c_str());

  // Save extra settings
  settings.mdnsName = String(mdnsParam.getValue());
  saveSettings();
}

void mdnsSetup() {
  if (!MDNS.begin(settings.mdnsName.c_str())) {
    Serial.println("[mDNS] Error setting up mDNS responder!");
  } else {
    Serial.print("[mDNS] mDNS responder started as ");
    Serial.print(settings.mdnsName);
    Serial.println(".local");
    MDNS.addService("_coap", "_udp", 5683);
  }
}

// Settings
void loadSettings() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[FS] Failed to mount SPIFFS");
    return;
  }
  if (!SPIFFS.exists(SETTINGS_PATH)) {
    Serial.println("[FS] settings.json not found, creating default");
    saveSettings();
    return;
  }
  File file = SPIFFS.open(SETTINGS_PATH, "r");
  if (!file) {
    Serial.println("[FS] Failed to open settings.json");
    return;
  }
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, file);
  if (err) {
    Serial.println("[FS] Failed to parse settings.json, using defaults");
    file.close();
    saveSettings();
    return;
  }
  settings.mdnsName = doc["mdnsName"] | settings.mdnsName;
  file.close();
}

void saveSettings() {
  File file = SPIFFS.open(SETTINGS_PATH, "w");
  if (!file) {
    Serial.println("[FS] Failed to open settings.json for writing");
    return;
  }
  StaticJsonDocument<256> doc;
  doc["mdnsName"] = settings.mdnsName;
  serializeJsonPretty(doc, file);
  file.close();
}


// =-------------------------------------------------------------------------------------= Main =--=

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); } // Wait for serial port to be ready (important for some boards)

  loadSettings();
  animationSetup();
  wifiSetup();
  mdnsSetup();
  coapSetup();

  statusMode = STATUS_READY;
}

void loop() {
  animationLoop();
  coapLoop();
}
