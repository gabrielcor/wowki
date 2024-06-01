#include <FastLED.h>
#define NUM_LEDS 15 
#define DATA_PINA 18 // Connect to the data wires on the pixel strips
#define DATA_PINB 17
#define DATA_PINC 5
#define BUTTON_PIN  26

bool gameStarted = true;
bool buttonPressed = false;
CRGB ledsA[NUM_LEDS]; // sets number of pixels that will light on each strip.
CRGB ledsB[NUM_LEDS];
CRGB ledsC[NUM_LEDS];

// LED control variables
int ledIndex = 0;
bool increasing = true;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 500; 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
  FastLED.addLeds<WS2812B, DATA_PINA, GRB>(ledsA, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PINB, GRB>(ledsB, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PINC, GRB>(ledsC, NUM_LEDS);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  randomSeed(4); 
}

void SendLedCountToApi(int index)
{
  return;
}


void updateLeds() {
  if (increasing) {
    ledIndex++;
    if (ledIndex >= NUM_LEDS) {
      increasing = false;
    }
  } else {
    ledIndex--;
    if (ledIndex <= 0) {
      increasing = true;
    }
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < ledIndex) {
      ledsA[i] = CRGB::White;  // Set color to white
    } else {
      ledsA[i] = CRGB::Black;  // Turn off
    }
  }
  FastLED.show();
}

void loop() {


  if (gameStarted) {
    // Check button press
    if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
      buttonPressed = true;
      SendLedCountToApi(ledIndex);
      gameStarted = false;  // Stop the game after sending the count
    } else if (digitalRead(BUTTON_PIN) == HIGH) {
      buttonPressed = false;
    }

    // Update LED sequence
    if (millis() - lastUpdate > updateInterval) {
      lastUpdate = millis();
      updateLeds();
    }
  }

  }


/*
void fillStrip(int number, const struct CRGB &color) {
  switch (number) {
    case 1:
    fill_solid(ledsA, NUM_LEDS, color);
    break;
    case 2:
    fill_solid(ledsB, NUM_LEDS, color);
    break;
    case 3:
    fill_solid(ledsC, NUM_LEDS, color);
    break;
  }
    FastLED.show();
}    
*/



