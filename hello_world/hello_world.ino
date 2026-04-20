#include <FastLED.h>

// --- Pin Definitions ---
#define LED_PIN           18
#define TOUCH_PIN         4
#define SENSOR_POWER_PIN  15 // The GPIO pin acting as 3.3V power for the sensor

// --- LED Setup ---
#define NUM_LEDS          12 
CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200); 
  
  // 1. Initialize the Power Hack FIRST
  // This turns Pin 15 into a constant 3.3V power supply for the touch sensor
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, HIGH);
  // Tell FastLED you are on 3.3V and limit it to roughly 500mA to be safe
  FastLED.setMaxPowerInVoltsAndMilliamps(3, 500);
  
  // Give the sensor a tiny fraction of a second to power up and stabilize
  delay(10);

  // 2. Initialize the Touch Sensor
  // Using PULLDOWN keeps the pin grounded (LOW) when no one is touching it
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  // 3. Initialize the LED Ring
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  
  // Set a safe brightness limit (0-255) for USB / 3.3V power
  FastLED.setBrightness(255); 
  
  FastLED.clear();
  FastLED.show();
  
  Serial.println("Hardware initialized. Waiting for touch...");
}

void loop() {
  int touchState = digitalRead(TOUCH_PIN);

  if (touchState == HIGH) {
    // Fill the entire ring with a color
    fill_solid(leds, NUM_LEDS, CRGB::Red); 
    Serial.println("Sensor touched! Illuminating ring...");
  } else {
    // Turn the whole ring off when not touched
    FastLED.clear(); 
  }

  FastLED.show();
  delay(50); 
}
