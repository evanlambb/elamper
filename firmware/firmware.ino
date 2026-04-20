#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// ===================== CONFIGURATION =====================
// Update these before flashing each lamp.
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* WS_HOST       = "elamper.onrender.com";  // no https://, no trailing slash
const uint16_t WS_PORT    = 443;
const char* ROOM_ID       = "pair-alpha";

// ===================== HARDWARE PINS =====================
#define LED_PIN           18
#define TOUCH_PIN         4
#define SENSOR_POWER_PIN  15

// ===================== LED SETUP =========================
#define NUM_LEDS          12
CRGB leds[NUM_LEDS];

// ===================== COLORS ============================
struct NamedColor {
  const char* name;
  CRGB       value;
};

const NamedColor COLORS[] = {
  { "red",   CRGB::Red   },
  { "green", CRGB::Green },
  { "blue",  CRGB::Blue  },
};
const int NUM_COLORS = sizeof(COLORS) / sizeof(COLORS[0]);

// ===================== STATE =============================
bool powerOn       = false;
int  colorIndex    = 0;
bool stateChanged  = false;

// ===================== TOUCH DETECTION ===================
bool     lastTouchState   = false;
unsigned long pressStart   = 0;
const unsigned long LONG_PRESS_MS = 1000;
const unsigned long DEBOUNCE_MS   = 50;
unsigned long lastDebounce = 0;

// ===================== WEBSOCKET =========================
WebSocketsClient wsClient;
bool wsConnected = false;

// ---------------------------------------------------------
int colorIndexByName(const char* name) {
  for (int i = 0; i < NUM_COLORS; i++) {
    if (strcmp(COLORS[i].name, name) == 0) return i;
  }
  return 0;
}

void applyLEDs() {
  if (powerOn) {
    fill_solid(leds, NUM_LEDS, COLORS[colorIndex].value);
  } else {
    FastLED.clear();
  }
  FastLED.show();
}

void sendState() {
  if (!wsConnected) {
    Serial.println("(not connected, skipping send)");
    return;
  }

  StaticJsonDocument<64> doc;
  doc["power"] = powerOn;
  doc["color"] = COLORS[colorIndex].name;

  char buf[64];
  size_t n = serializeJson(doc, buf);
  wsClient.sendTXT(buf, n);

  Serial.printf("TX: %s\n", buf);
}

void handleIncoming(const char* payload) {
  Serial.printf("RX: %s\n", payload);

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, payload)) return;

  if (doc.containsKey("power") && doc.containsKey("color")) {
    bool newPower = doc["power"];
    const char* newColor = doc["color"];
    int newIndex = colorIndexByName(newColor);

    if (newPower != powerOn || newIndex != colorIndex) {
      powerOn    = newPower;
      colorIndex = newIndex;
      if (!powerOn) colorIndex = 0;
      applyLEDs();
      Serial.printf("State updated -> power=%d color=%s\n", powerOn, COLORS[colorIndex].name);
    }
  }
}

void onWsEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      wsConnected = true;
      Serial.printf("WebSocket connected to %s\n", (char*)payload);
      break;
    case WStype_DISCONNECTED:
      wsConnected = false;
      Serial.println("WebSocket disconnected");
      break;
    case WStype_TEXT:
      handleIncoming((char*)payload);
      break;
    case WStype_ERROR:
      Serial.printf("WebSocket error: %s\n", (char*)payload);
      break;
    default:
      break;
  }
}

void connectWiFi() {
  Serial.printf("Connecting to WiFi '%s'", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected  IP: %s\n", WiFi.localIP().toString().c_str());
}

// ======================== SETUP ==========================
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, HIGH);
  FastLED.setMaxPowerInVoltsAndMilliamps(3, 500);
  delay(10);

  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  FastLED.clear();
  FastLED.show();

  connectWiFi();

  String path = String("/ws/lamps/") + ROOM_ID;
  Serial.printf("Connecting WebSocket: wss://%s:%u%s\n", WS_HOST, WS_PORT, path.c_str());

  wsClient.beginSSL(WS_HOST, WS_PORT, path.c_str());
  wsClient.onEvent(onWsEvent);
  wsClient.setReconnectInterval(3000);
  wsClient.enableHeartbeat(15000, 3000, 2);
}

// ======================== LOOP ===========================
void loop() {
  wsClient.loop();

  // --- Read touch with debounce ---
  bool raw = digitalRead(TOUCH_PIN) == HIGH;
  if (millis() - lastDebounce < DEBOUNCE_MS) raw = lastTouchState;
  else lastDebounce = millis();

  bool pressed  = raw && !lastTouchState;
  bool released = !raw && lastTouchState;

  if (pressed) {
    pressStart = millis();
  }

  if (released) {
    unsigned long duration = millis() - pressStart;

    if (duration >= LONG_PRESS_MS) {
      powerOn = !powerOn;
      colorIndex = 0;
      stateChanged = true;
      Serial.printf("Long press -> power=%d\n", powerOn);

    } else if (powerOn) {
      colorIndex = (colorIndex + 1) % NUM_COLORS;
      stateChanged = true;
      Serial.printf("Short press -> color=%s\n", COLORS[colorIndex].name);
    }
  }

  lastTouchState = raw;

  if (stateChanged) {
    stateChanged = false;
    applyLEDs();
    sendState();
  }
}
