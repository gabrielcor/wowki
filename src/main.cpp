#include <FastLED.h>
#include <WiFi.h>
#include <HTTPClient.h>
#define NUM_LEDS 20
#define DATA_PINA 17 // Connect to the data wires on the pixel strips
#define DATA_PINB 19
#define DATA_PINC 16
// Si son 3 se sugiere 5,25,26
#define BUTTON_PINA 5
#define BUTTON_PINB 5
#define BUTTON_PINC 5

bool gameStarted = true;

CRGB ledsA[NUM_LEDS]; // sets number of pixels that will light on each strip.
CRGB ledsB[NUM_LEDS];
CRGB ledsC[NUM_LEDS];

// To check for button pressed but not enable it again until it is un pressed

const char *url2SendResult = "https://eovunmo8a5u8h34.m.pipedream.net";
/*
const char* ssid = "Wokwi-GUEST";
const char* password = "";
*/
const char *ssid = "blackcrow_01";
const char *password = "8001017170";
// LED control variables
#define CANT_STRIPS 3;
int ledIndexes[3];
CRGB ledColors[3];
bool alreadySelected[3];
int buttonPins[3];
bool buttonPressed[3];

bool increasing[3];
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 50;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  for (int i = 0; i < 3; i++)
  {
    ledIndexes[i] = 0;
    increasing[i] = true;
    alreadySelected[i] = false;
    buttonPressed[i] = false;
  }
  ledColors[0] = CRGB::Green;
  ledColors[1] = CRGB::Blue;
  ledColors[2] = CRGB::Orange;
  buttonPins[0] = BUTTON_PINA;
  buttonPins[1] = BUTTON_PINB;
  buttonPins[2] = BUTTON_PINC;

  Serial.println("Hello, ESP32!");
  FastLED.addLeds<WS2812B, DATA_PINA, GRB>(ledsA, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PINB, GRB>(ledsB, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PINC, GRB>(ledsC, NUM_LEDS);
  pinMode(BUTTON_PINA, INPUT_PULLUP);
  pinMode(BUTTON_PINB, INPUT_PULLUP);
  pinMode(BUTTON_PINC, INPUT_PULLUP);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void updateLeds(CRGB *led2Update, int stripNumber)
{
  int paintTo = 0;
  if (!alreadySelected[stripNumber])
  {
    if (increasing[stripNumber])
    {

      ledIndexes[stripNumber]++;
      if (ledIndexes[stripNumber] >= NUM_LEDS)
      {
        increasing[stripNumber] = false;
      }
    }
    else
    {

      ledIndexes[stripNumber]--;
      if (ledIndexes[stripNumber] <= 0)
      {
        increasing[stripNumber] = true;
      }
    }
  }
  paintTo = ledIndexes[stripNumber];
  for (size_t i = 0; i < NUM_LEDS; i++)
  {
    if (i < paintTo)
      led2Update[i] = ledColors[stripNumber];
    else
      led2Update[i] = CRGB::Black;
  }
}

void sendLedCountToApi(int countA, int countB, int countC)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(url2SendResult); // Replace with your API endpoint
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"ledA\":" + String(countA) + ",\"ledB\":" + String(countB) + ",\"ledC\":" + String(countC) + "}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    }
    else
    {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }
    http.end();
  }
  else
  {
    Serial.println("WiFi not connected");
  }
}

void CheckButtonUnPress()
{
  for (size_t i = 0; i < 3; i++)
  {
    if (buttonPressed[i] && digitalRead(buttonPins[i]) == HIGH)
    {
      // Debounce
      delay(50);
      if (digitalRead(buttonPins[i]) == HIGH)
      {
        buttonPressed[i] = false;
        Serial.print("Button unpressed: ");
        Serial.println(i);
      }
    }
  }
}

void CheckButtonPress()
{
  for (size_t i = 0; i < 3; i++)
  {
    if (!buttonPressed[i] && digitalRead(buttonPins[i]) == LOW)
    {
      // Debounce
      delay(50);
      if (digitalRead(buttonPins[i]) == LOW)
      {
        buttonPressed[i] = true;
        Serial.print("Button pressed: ");
        Serial.println(i);

        if (!increasing[i]) // if we are decreasing, it's one less
        {
          ledIndexes[i]--;
        }
        alreadySelected[i] = true;

        // Exit the loop as soon as the first button press is detected
        break;
      }
    }
  }
}

void CheckEndGame()
{
  bool gameEnded = true;
  for (size_t i = 0; i < 3; i++)
  {
    if (!alreadySelected[i])
    {
      gameEnded = false;
    }
  }
  if (gameEnded)
  {
    gameStarted = false;
    int countA = ledIndexes[0];
    int countB = ledIndexes[1];
    int countC = ledIndexes[2];
    String payload = "{\"ledA\":" + String(countA) + ",\"ledB\":" + String(countB) + ",\"ledC\":" + String(countC) + "}";
    Serial.println("Seleccionado");
    Serial.println(payload);
  }
}

void loop()
{

  if (gameStarted)
  {
    CheckButtonPress();
    CheckButtonUnPress();
    // Update LED sequence
    if ((millis() - lastUpdate) > updateInterval)
    {

      lastUpdate = millis();
      updateLeds(ledsA, 0);
      updateLeds(ledsB, 1);
      updateLeds(ledsC, 2);
      FastLED.show();
    }
    CheckEndGame();
  }
}
