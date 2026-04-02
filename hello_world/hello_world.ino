#include <FastLED.h>

// --- Pin Definitions ---
#define LED_PIN     16
#define TOUCH_PIN   4

// --- LED Setup ---
#define NUM_LEDS    12 // Updated for the ring
CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200); 
  
  pinMode(TOUCH_PIN, INPUT);

  // Initialize FastLED for the 12-LED ring
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  
  // Set a safe brightness limit (0-255) for USB power
  FastLED.setBrightness(50); 
  
  FastLED.clear();
  FastLED.show();
  
  Serial.println("Hardware initialized. Waiting for touch...");
}

void loop() {
  int touchState = digitalRead(TOUCH_PIN);

  if (touchState == HIGH) {
    // Fill the entire ring with a color (using Purple here, but change as you like!)
    fill_solid(leds, NUM_LEDS, CRGB::Purple); 
    Serial.println("Sensor touched! Illuminating ring...");
  } else {
    // Turn the whole ring off when not touched
    FastLED.clear(); 
  }

  FastLED.show();
  delay(50); 
}