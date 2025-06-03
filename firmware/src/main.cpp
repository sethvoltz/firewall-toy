#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>


// =---------------------------------------------------------------------------------= Settings =--=

#define LED_PIN                     D1
#define NUM_LEDS                    3
#define BRIGHTNESS                  128
#define ANIMATION_MS                33
#define SETTINGS_PATH               "/settings.json"
#define FLAME_BLEND_STEPS           6
#define BRIGHTNESS_BLEND_STEPS      150
#define WEBSOCKET_PUBLISH_MS        500

// =----------------------------------------------------------------------------------= Structs =--=

enum AnimationMode {
  ANIMATION_STATIC,
  ANIMATION_FLAME
};

struct Settings {
  String mdnsName = "firewalltoy";
};

struct BrightnessConfig {
  bool nightEnabled = false;
  int nightStartHour = 22; // 22:00
  int nightEndHour = 7;   // 07:00
  uint8_t dayBrightness = 128;
  uint8_t nightBrightness = 32;
};

struct HSV {
  float h, s, v;
};


// =-------------------------------------------------------------------------------= Signatures =--=

// TODO: Move to header file
void filesystemSetup();
void animationSetup();
void animationLoop();
void wifiSetup();
void mdnsSetup();
void loadSettings();
void saveSettings();
void setStatusColor(uint8_t r, uint8_t g, uint8_t b);
HSV lerpHSV(const HSV& c1, const HSV& c2, float t);
HSV flameColor(const HSV& base, float h_jitter, float s_jitter, float v_jitter);
HSV rgbToHsv(uint8_t r, uint8_t g, uint8_t b);
void httpSetup();
void handleApiPostRequest(AsyncWebServerRequest *request);
void handleApiPostMode(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handleApiGetBrightness(AsyncWebServerRequest *request);
void handleApiPostBrightness(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void setModeAndColorFromJson(JsonVariantConst doc);
void ntpSetup();
void ntpLoop();
void wsSetup();
void wsLoop();
void broadcastLedState();
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);


// =----------------------------------------------------------------------------------= Globals =--=

Settings settings;
BrightnessConfig brightnessConfig;

// Network
bool wifiFeaturesEnabled = false;

// NTP Client
WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp, "pool.ntp.org", 0, 60000); // UTC, update every 60s

// AsyncWebServer and WebSocket
static AsyncWebServer asyncServer(80);
static AsyncWebSocket ws("/ws");

// Animation
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
AnimationMode currentMode = ANIMATION_FLAME;
uint8_t currentR = 255, currentG = 110, currentB = 15;

// Flame animation state
HSV currentColors[NUM_LEDS];
HSV targetColors[NUM_LEDS];
uint8_t flameStep = 0;

Preferences preferences;


// =--------------------------------------------------------------------------------= Functions =--=

// Linear interpolation between two HSV colors, handling hue wraparound
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

// Generate a random float between a and b, similar to Python's random.uniform(a, b)
// Note: uniform detects the number of decimals of the floats and randomizes that range. This hard
// codes to 2 places to match expected behavior
float randomFloat(float a, float b) {
  return a + ((b - a) * random(0, 101) / 100.0);
}

// Jitter a base HSV color for flame effect
HSV flameColor(const HSV& base, float h_jitter = 0.01f, float s_jitter = 0.05f, float v_jitter = 0.4f) {
  HSV out = base;
  out.h = fmodf(base.h + randomFloat(-h_jitter, h_jitter), 1.0f);
  out.s = fminf(1.0f, fmaxf(0.0f, base.s + randomFloat(-s_jitter, s_jitter)));
  out.v = fminf(1.0f, fmaxf(0.0f, base.v + randomFloat(-v_jitter, v_jitter)));
  return out;
}

// Convert RGB (0-255) to HSV (0-1)
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

  setStatusColor(0, 255, 255);
}

void animationLoop() {
  static unsigned long lastUpdate = 0;
  static float currentBrightness = BRIGHTNESS;
  static unsigned long lastBroadcast = 0; // Throttle WebSocket broadcasts
  unsigned long now = millis();

  // Determine target brightness based on day/night config and NTP time
  uint8_t targetBrightness = brightnessConfig.dayBrightness;
  if (brightnessConfig.nightEnabled && timeClient.isTimeSet()) {
    int hour = timeClient.getHours();
    bool isNight = false;
    if (brightnessConfig.nightStartHour < brightnessConfig.nightEndHour) {
      // Night does not cross midnight
      isNight = (hour >= brightnessConfig.nightStartHour && hour < brightnessConfig.nightEndHour);
    } else {
      // Night crosses midnight
      isNight = (hour >= brightnessConfig.nightStartHour || hour < brightnessConfig.nightEndHour);
    }
    if (isNight) {
      targetBrightness = brightnessConfig.nightBrightness;
    }
  }

  // Tween brightness
  float step = (targetBrightness - currentBrightness) / BRIGHTNESS_BLEND_STEPS;
  if (fabs(step) < 0.5f) {
    currentBrightness = targetBrightness;
  } else {
    currentBrightness += step;
  }
  strip.setBrightness((uint8_t)roundf(currentBrightness));

  if (now - lastUpdate >= ANIMATION_MS) {
    lastUpdate = now;

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
        uint32_t color = strip.ColorHSV(
          (uint16_t)(blended.h * 65535.0f),
          (uint8_t)(blended.s * 255.0f),
          (uint8_t)(blended.v * 255.0f)
        );
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
    // After updating LEDs, broadcast state (throttled)
    if (now - lastBroadcast >= WEBSOCKET_PUBLISH_MS) {
      broadcastLedState();
      lastBroadcast = now;
    }
  }
}

void setStatusColor(uint8_t r, uint8_t g, uint8_t b) {
  strip.fill(strip.Color(r, g, b));
  strip.show();
}


// WIFI & mDNS
void wifiSetup() {
  setStatusColor(0, 0, 255);

  Serial.println("[WiFi] Starting WiFiManager for captive portal if needed...");
  WiFiManager wifiManager;
  WiFiManagerParameter mdnsParam("mdns", "mDNS Name", settings.mdnsName.c_str(), 32);
  wifiManager.addParameter(&mdnsParam);

  String apName = "FirewallToy-Setup-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  wifiManager.setConfigPortalTimeout(180); // 3 min timeout
  wifiManager.setEnableConfigPortal(true); // Always allow config portal

  // Set status to portal mode if config portal is started
  wifiManager.setAPCallback([](WiFiManager*) {
    setStatusColor(255, 0, 255);
  });
  wifiManager.autoConnect(apName.c_str());
  wifiFeaturesEnabled = true;

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
    MDNS.addService("_http", "_tcp", 80);
  }
}

// Filesystem
void filesystemSetup() {
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] Failed to mount LittleFS");
    return;
  }
}

// Settings
void loadSettings() {
  preferences.begin("firewall-toy", true); // read-only

  settings.mdnsName = preferences.getString("mdnsName", settings.mdnsName);

  // Load brightness config
  brightnessConfig.nightEnabled = preferences.getBool("nightEnabled", brightnessConfig.nightEnabled);
  brightnessConfig.nightStartHour = preferences.getInt("nightStartHour", brightnessConfig.nightStartHour);
  brightnessConfig.nightEndHour = preferences.getInt("nightEndHour", brightnessConfig.nightEndHour);
  brightnessConfig.dayBrightness = preferences.getUChar("dayBrightness", brightnessConfig.dayBrightness);
  brightnessConfig.nightBrightness = preferences.getUChar("nightBrightness", brightnessConfig.nightBrightness);

  preferences.end();
}

void saveSettings() {
  preferences.begin("firewall-toy", false); // read+write

  preferences.putString("mdnsName", settings.mdnsName);
  preferences.putBool("nightEnabled", brightnessConfig.nightEnabled);
  preferences.putInt("nightStartHour", brightnessConfig.nightStartHour);
  preferences.putInt("nightEndHour", brightnessConfig.nightEndHour);
  preferences.putUChar("dayBrightness", brightnessConfig.dayBrightness);
  preferences.putUChar("nightBrightness", brightnessConfig.nightBrightness);

  preferences.end();
}

void setModeAndColorFromJson(JsonVariantConst doc) {
  // Expecting: {"mode": "static"|"flame", "color": {"r":0-255, "g":0-255, "b":0-255}}
  const char* mode = doc["mode"] | "static";
  if (strcmp(mode, "static") == 0) {
    currentMode = ANIMATION_STATIC;
  } else if (strcmp(mode, "flame") == 0) {
    currentMode = ANIMATION_FLAME;
  }

  // Only update color if present
  if (doc.containsKey("color")) {
    JsonVariantConst color = doc["color"];
    if (color.containsKey("r")) currentR = color["r"];
    if (color.containsKey("g")) currentG = color["g"];
    if (color.containsKey("b")) currentB = color["b"];
  }

  Serial.printf("[Mode/Color] Mode: %s, Color: r=%d, g=%d, b=%d\n", mode, currentR, currentG, currentB);
}

// API Handlers
void handleApiPostRequest(AsyncWebServerRequest *request) {
  String path = request->url();
  Serial.printf("[HTTP] %s called\n", path.c_str());
}

void handleApiPostMode(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  Serial.println("[HTTP] /api/mode called");
  String body = String((const char*)data, len);
  Serial.print("[HTTP] Mode body: ");
  Serial.println(body);
  StaticJsonDocument<192> doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.println("[HTTP] Invalid JSON received in /api/mode");
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  setModeAndColorFromJson(doc.as<JsonVariantConst>());
  request->send(200, "application/json", "{\"status\":\"OK\"}");
}

void handleApiGetBrightness(AsyncWebServerRequest *request) {
  StaticJsonDocument<192> doc;

  doc["nightEnabled"] = brightnessConfig.nightEnabled;
  doc["nightStartHour"] = brightnessConfig.nightStartHour;
  doc["nightEndHour"] = brightnessConfig.nightEndHour;
  doc["dayBrightness"] = brightnessConfig.dayBrightness;
  doc["nightBrightness"] = brightnessConfig.nightBrightness;

  String out;
  serializeJson(doc, out);
  request->send(200, "application/json", out);
}

void handleApiPostBrightness(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  String body = String((const char*)data, len);
  StaticJsonDocument<192> doc;
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  if (doc.containsKey("nightEnabled")) brightnessConfig.nightEnabled = doc["nightEnabled"];
  if (doc.containsKey("nightStartHour")) brightnessConfig.nightStartHour = doc["nightStartHour"];
  if (doc.containsKey("nightEndHour")) brightnessConfig.nightEndHour = doc["nightEndHour"];
  if (doc.containsKey("dayBrightness")) brightnessConfig.dayBrightness = doc["dayBrightness"];
  if (doc.containsKey("nightBrightness")) brightnessConfig.nightBrightness = doc["nightBrightness"];

  saveSettings();
  request->send(200, "application/json", "{\"status\":\"OK\"}");
}

// HTTP Server
void httpSetup() {
  asyncServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html", false);
  });

  asyncServer.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/favicon.png", "image/png", false);
  });

  // Serve all /_app/* files dynamically from LittleFS
  asyncServer.onNotFound([](AsyncWebServerRequest *request) {
    String path = request->url();
    Serial.printf("[HTTP] Not found handler: %s\n", path.c_str());

    if (path.startsWith("/_app/")) {
      String contentType = "application/octet-stream";
      if (path.endsWith(".js")) contentType = "application/javascript";
      else if (path.endsWith(".css")) contentType = "text/css";
      else if (path.endsWith(".json")) contentType = "application/json";
      else if (path.endsWith(".png")) contentType = "image/png";
      else if (path.endsWith(".ico")) contentType = "image/x-icon";
      else if (path.endsWith(".html")) contentType = "text/html";

      request->send(LittleFS, path, contentType, false);
      return;
    }

    // Fallback: serve index.html for SPA routing
    request->send(LittleFS, "/index.html", "text/html", false);
  });

  asyncServer.on("/api/brightness", HTTP_GET, handleApiGetBrightness);
  asyncServer.on("/api/mode", HTTP_POST, handleApiPostRequest, nullptr, handleApiPostMode);
  asyncServer.on("/api/brightness", HTTP_GET, handleApiGetBrightness);
  asyncServer.on("/api/brightness", HTTP_POST, handleApiPostRequest, nullptr, handleApiPostBrightness);

  Serial.println("[HTTP] Async web server started on port 80");
}

void wsSetup() {
  ws.onEvent(onWsEvent);
  asyncServer.addHandler(&ws);
  asyncServer.begin();
  Serial.println("[WS] AsyncWebSocket server started on /ws");
}

void wsLoop() {
  ws.cleanupClients();
}

void broadcastLedState() {
  StaticJsonDocument<384> doc;

  doc["brightness"] = strip.getBrightness();

  JsonArray leds = doc.createNestedArray("leds");
  for (int i = 0; i < NUM_LEDS; i++) {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;
    JsonObject led = leds.createNestedObject();
    led["r"] = r;
    led["g"] = g;
    led["b"] = b;
  }
  
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // Send initial state on connect
    broadcastLedState();
  }
}

// NTP Client
void ntpSetup() {
  timeClient.begin();
  if (timeClient.forceUpdate()) {
    Serial.print("[NTP] Time: ");
    Serial.println(timeClient.getFormattedTime());
  } else {
    Serial.println("[NTP] Failed to get time");
  }
}

void ntpLoop() {
  timeClient.update();
}

// =-------------------------------------------------------------------------------------= Main =--=

void setup() {
  Serial.begin(115200);
  // while (!Serial) { delay(10); } // Uncomment this if you need serial output early

  filesystemSetup();
  loadSettings();
  animationSetup();
  wifiSetup();

  if (wifiFeaturesEnabled) {
    mdnsSetup();
    httpSetup();
    ntpSetup();
    wsSetup();
  }
}

void loop() {
  animationLoop();

  if (wifiFeaturesEnabled) {
    ntpLoop();
    wsLoop(); // Handle AsyncWebSocket cleanup
  }
}
