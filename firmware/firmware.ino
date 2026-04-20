#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <FastLED.h>

using namespace websockets;

// ===================== CONFIGURATION =====================
// Update these before flashing each lamp.
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* WS_HOST       = "YOUR_SERVER.onrender.com";
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
WebsocketsClient wsClient;
bool wsConnected           = false;
unsigned long lastReconnect = 0;
const unsigned long RECONNECT_INTERVAL_MS = 3000;

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
  if (!wsConnected) return;

  StaticJsonDocument<64> doc;
  doc["power"] = powerOn;
  doc["color"] = COLORS[colorIndex].name;

  char buf[64];
  serializeJson(doc, buf);
  wsClient.send(buf);

  Serial.printf("TX: %s\n", buf);
}

void onMessage(WebsocketsMessage msg) {
  Serial.printf("RX: %s\n", msg.data().c_str());

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, msg.data())) return;

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

void onEvent(WebsocketsEvent event, String data) {
  switch (event) {
    case WebsocketsEvent::ConnectionOpened:
      wsConnected = true;
      Serial.println("WebSocket connected");
      break;
    case WebsocketsEvent::ConnectionClosed:
      wsConnected = false;
      Serial.println("WebSocket disconnected");
      break;
    case WebsocketsEvent::GotPing:
      wsClient.pong();
      break;
    default:
      break;
  }
}

void connectWebSocket() {
  String url = String("wss://") + WS_HOST + "/ws/lamps/" + ROOM_ID;
  Serial.printf("Connecting to %s ...\n", url.c_str());
  wsClient.connect(url);
}

void connectWiFi() {
  Serial.printf("Connecting to WiFi '%s'", WIFI_SSID);
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

  wsClient.onMessage(onMessage);
  wsClient.onEvent(onEvent);
  connectWebSocket();
}

// ======================== LOOP ===========================
void loop() {
  wsClient.poll();

  // --- Reconnect if needed ---
  if (!wsConnected && millis() - lastReconnect > RECONNECT_INTERVAL_MS) {
    lastReconnect = millis();
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    connectWebSocket();
  }

  // --- Read touch with debounce ---
  bool raw = digitalRead(TOUCH_PIN) == HIGH;
  if (millis() - lastDebounce < DEBOUNCE_MS) raw = lastTouchState;
  else lastDebounce = millis();

  bool pressed  = raw && !lastTouchState;   // rising edge
  bool released = !raw && lastTouchState;    // falling edge

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

  // --- Push state if it changed ---
  if (stateChanged) {
    stateChanged = false;
    applyLEDs();
    sendState();
  }
}
