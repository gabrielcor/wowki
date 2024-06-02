#include <FastLED.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>

#define NUM_LEDS 20
#define DATA_PINA 17 // Connect to the data wires on the pixel strips
#define DATA_PINB 19
#define DATA_PINC 16
// Si son 3 se sugiere 5,25,26
#define BUTTON_PINA 5
#define BUTTON_PINB 5
#define BUTTON_PINC 5

bool gameStarted = false;

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
unsigned long updateInterval = 50;
AsyncWebServer server(80);

void postRule(AsyncWebServerRequest *request, uint8_t *data)
{
  size_t len = request->contentLength();
  Serial.println("Data: ");
  Serial.write(data, len); // Correctly print the received data
  Serial.println("\nLength: ");
  Serial.println(len);

  // Construct the received data string with the specified length
  String receivedData = "";
  for (size_t i = 0; i < len; i++)
  {
    receivedData += (char)data[i];
  }

  Serial.println("Received Data String: " + receivedData);

  if (receivedData.indexOf("start") != -1)
  {
    gameStarted = true;
    for (int i = 0; i < 3; i++)
    {
      ledIndexes[i] = 0;
      increasing[i] = true;
      alreadySelected[i] = false;
      buttonPressed[i] = false;
    }
    request->send(200, "application/json", "{\"status\":\"game started\"}");
    Serial.println("Command received: start");
  }
  else if (receivedData.indexOf("shutdown") != -1)
  {
    gameStarted = false;
    FastLED.clear();
    FastLED.show();
    request->send(200, "application/json", "{\"status\":\"game shutdown\"}");
    Serial.println("Command received: shutdown");
  }
  else if (receivedData.indexOf("status") != -1)
  {
    if (gameStarted)
    {
      request->send(200, "application/json", "{\"status\":\"game started\"}");
    }
    else
    {
      request->send(200, "application/json", "{\"status\":\"game shutdown\"}");
    }
    Serial.println("Command received: status");
  }
  else if (receivedData.indexOf("updateInterval=")!=-1)
  {
      int startIndex = receivedData.indexOf("updateInterval=") + 15;
    int endIndex = receivedData.indexOf(' ', startIndex); // Assuming commands are space-separated

    if (endIndex == -1) {
      endIndex = receivedData.length();
    }

    String intervalStr = receivedData.substring(startIndex, endIndex);
    int intervalValue = intervalStr.toInt();

    if (intervalValue >= 10 && intervalValue <= 500)
    {
      updateInterval = intervalValue;
      request->send(200, "application/json", "{\"status\":\"update interval set\"}");
      Serial.println("Command received: updateInterval=" + String(updateInterval));
    }
  }
  else
  {
    request->send(400, "application/json", "{\"status\":\"invalid command\"}");
    Serial.println("Invalid command");
  }
}
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

  // Print the IP address
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/api/command", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            { postRule(request, data); });

  server.begin();
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
  bool isPressed = false;
  for (size_t i = 0; i < 3; i++)
  {
    if (buttonPressed[i])
    {
      isPressed = true;
    }
  }
  if (!isPressed)
  {
    for (size_t i = 0; i < 3; i++)
    {
      if (!buttonPressed[i] && digitalRead(buttonPins[i]) == LOW && !alreadySelected[i])
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
