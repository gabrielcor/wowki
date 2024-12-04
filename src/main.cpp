#include <FastLED.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>


#define NUM_LEDS 20 // number of leds of each strip 
#define DATA_PINA 17 // Connect to the data wires on the pixel strips
#define DATA_PINB 19
#define DATA_PINC 16
// pins for the buttons se sugiere 5,25,26
#define BUTTON_PINA 5
#define BUTTON_PINB 5
#define BUTTON_PINC 5

// Supported colors for the strips
std::map<String, CRGB> colorMap = {
    {"Red", CRGB::Red},
    {"Green", CRGB::Green},
    {"Blue", CRGB::Blue},
    {"Orange", CRGB::Orange},
    {"Yellow", CRGB::Yellow},
    {"Purple", CRGB::Purple},
    {"Cyan", CRGB::Cyan},
    {"White", CRGB::White}};

bool gameStarted = false;

CRGB ledsA[NUM_LEDS]; // sets number of pixels that will light on each strip.
CRGB ledsB[NUM_LEDS];
CRGB ledsC[NUM_LEDS];

// URL to send the result when the puzle is ready (all the strips selected)
const char *url2SendResult = "http://192.168.20.147:8123/api/events/bc_custom_event";
const char *bearerString = "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJiNjI1NzRjNmExYTg0MGE1OGFjZGQ4YTViMTBkYWY2OSIsImlhdCI6MTczMjgwMjkxMCwiZXhwIjoyMDQ4MTYyOTEwfQ.Cv7l7q76Sd6nWn3QweivugxsXh85ylk-mhQqtfKfbhs";

// #define WOWKI_EMULATION
#ifdef WOWKI_EMULATION

const char *ssid = "Wokwi-GUEST";
const char *password = "";
#else
const char *ssid = "blackcrow_01";
const char *password = "8001017170";
#endif

// LED control variables
int ledIndexes[3]; // number of leds ON for each strip (it moves until player selects)
CRGB ledColors[3]; // Colors of the strips
bool alreadySelected[3]; // To know if the player has selected a number of leds for this strip
int buttonPins[3]; // to hold the button pins
bool buttonPressed[3]; // To know if the button for a strip was pressed

bool increasing[3]; // indicates if the light in the strip is moving forward or backward
unsigned long lastUpdate = 0; // to handle update
unsigned long updateInterval = 50; // how frequent to update the moving strip
AsyncWebServer server(80); // to handle the published API

/// @brief Allows to show a hint, when invoked, stops the game makes an animation and shows the LED quantity received as parameters
/// @param countA How many LEDs to set to ON on Strip 0
/// @param countB How many LEDs to set to ON on Strip 1
/// @param countC How many LEDs to set to ON on Strip 2
void hintLeds(int countA, int countB, int countC)
{
  gameStarted = false;
  // Move forward and backward for each strip
  for (int i = 0; i < NUM_LEDS; i++)
  {
    for (int j = 0; j < NUM_LEDS; j++)
    {
      ledsA[j] = (j <= i) ? ledColors[0] : CRGB::Black;
      ledsB[j] = (j <= i) ? ledColors[1] : CRGB::Black;
      ledsC[j] = (j <= i) ? ledColors[2] : CRGB::Black;
    }
    FastLED.show();
    delay(updateInterval);
  }

  for (int i = NUM_LEDS - 1; i >= 0; i--)
  {
    for (int j = 0; j < NUM_LEDS; j++)
    {
      ledsA[j] = (j <= i) ? ledColors[0] : CRGB::Black;
      ledsB[j] = (j <= i) ? ledColors[1] : CRGB::Black;
      ledsC[j] = (j <= i) ? ledColors[2] : CRGB::Black;
    }
    FastLED.show();
    delay(updateInterval);
  }

  // Blink five times at the speed of updateInterval
  for (int k = 0; k < 5; k++)
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      ledsA[i] = CRGB::Black;
      ledsB[i] = CRGB::Black;
      ledsC[i] = CRGB::Black;
    }
    FastLED.show();
    delay(updateInterval);

    for (int i = 0; i < NUM_LEDS; i++)
    {
      ledsA[i] = ledColors[0];
      ledsB[i] = ledColors[1];
      ledsC[i] = ledColors[2];
    }
    FastLED.show();
    delay(updateInterval);
  }

  // Turn on the specific number of LEDs for each strip
  for (int i = 0; i < NUM_LEDS; i++)
  {
    ledsA[i] = (i < countA) ? ledColors[0] : CRGB::Black;
    ledsB[i] = (i < countB) ? ledColors[1] : CRGB::Black;
    ledsC[i] = (i < countC) ? ledColors[2] : CRGB::Black;
  }
  FastLED.show();
}

/// @brief Used to indicate win or lose blinks and shutdown
/// @param color 
void blinkAllLeds(int color)
{
  const int blinkDuration = 3000; // Total duration to blink in milliseconds
  const int blinkInterval = 100;  // Blink interval in milliseconds
  unsigned long startMillis = millis();
  bool ledState = false;

  CRGB colorToShow;
  if (color == 1)
    colorToShow = CRGB::Red;
  else
    colorToShow = CRGB::Green;

  while (millis() - startMillis < blinkDuration)
  {
    ledState = !ledState;
    for (int i = 0; i < NUM_LEDS; i++)
    {
      ledsA[i] = ledState ? colorToShow : CRGB::Black;
      ledsB[i] = ledState ? colorToShow : CRGB::Black;
      ledsC[i] = ledState ? colorToShow : CRGB::Black;
    }
    FastLED.show();
    delay(blinkInterval);
  }

  // Turn off all LEDs after blinking
  for (int i = 0; i < NUM_LEDS; i++)
  {
    ledsA[i] = CRGB::Black;
    ledsB[i] = CRGB::Black;
    ledsC[i] = CRGB::Black;
  }
  FastLED.show();
}

/// @brief Handle the API call
/// @param request full request
/// @param data JSON data
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
  else if (receivedData.indexOf("win") != -1)
  {
    gameStarted = false;
    FastLED.clear();
    FastLED.show();
    blinkAllLeds(2);
    request->send(200, "application/json", "{\"status\":\"game win\"}");
    Serial.println("Command received: win");
  }
  else if (receivedData.indexOf("lose") != -1)
  {
    gameStarted = false;
    FastLED.clear();
    FastLED.show();
    blinkAllLeds(1);
    request->send(200, "application/json", "{\"status\":\"game lose\"}");
    Serial.println("Command received: lose");
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
  else if (receivedData.indexOf("updateInterval=") != -1)
  {
    int startIndex = receivedData.indexOf("updateInterval=") + 15;
    int endIndex = receivedData.indexOf(' ', startIndex); // Assuming commands are space-separated

    if (endIndex == -1)
    {
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
  else if (receivedData.indexOf("ledColor") != -1)
  {
    int ledIndex = receivedData.substring(receivedData.indexOf("ledColor") + 8, receivedData.indexOf('=')).toInt();
    if (ledIndex < 0 || ledIndex > 2)
    {
      request->send(400, "application/json", "{\"status\":\"invalid led index\"}");
      Serial.println("Invalid LED index: " + String(ledIndex));
      return;
    }

    int startIndex = receivedData.indexOf('=') + 1;
    int endIndex = receivedData.indexOf(' ', startIndex); // Assuming commands are space-separated

    if (endIndex == -1)
    {
      endIndex = receivedData.length() - 2;
    }

    String colorStr = receivedData.substring(startIndex, endIndex);

    if (colorMap.find(colorStr) != colorMap.end())
    {
      ledColors[ledIndex] = colorMap[colorStr];
      request->send(200, "application/json", "{\"status\":\"ledColor" + String(ledIndex) + " set\"}");
      Serial.println("Command received: ledColor" + String(ledIndex) + "=" + colorStr);
    }
    else
    {
      request->send(400, "application/json", "{\"status\":\"invalid color value\"}");
      Serial.println("Invalid color value: " + colorStr);
    }
  }
  else if (receivedData.indexOf("hint") != -1)
  {
    int startIndex = receivedData.indexOf("hint=") + 5;
    int endIndex = receivedData.indexOf(' ', startIndex); // Assuming commands are space-separated

    if (endIndex == -1)
    {
      endIndex = receivedData.length();
    }

    String hintStr = receivedData.substring(startIndex, endIndex);
    int x, y, z;
    sscanf(hintStr.c_str(), "%d,%d,%d", &x, &y, &z);

    hintLeds(x, y, z);
    request->send(200, "application/json", "{\"status\":\"hint executed\"}");
    Serial.println("Command received: hint with values=" + hintStr);
  }
  else
  {
    request->send(400, "application/json", "{\"status\":\"invalid command\"}");
    Serial.println("Invalid command");
  }
}

/// @brief Initialize the game
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

  // Default colors for the strips
  ledColors[0] = CRGB::Green;
  ledColors[1] = CRGB::Blue;
  ledColors[2] = CRGB::Orange;
  buttonPins[0] = BUTTON_PINA;
  buttonPins[1] = BUTTON_PINB;
  buttonPins[2] = BUTTON_PINC;

  Serial.println("Hello, ESP32!");
  // LED Setup
  FastLED.addLeds<WS2812B, DATA_PINA, GRB>(ledsA, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PINB, GRB>(ledsB, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PINC, GRB>(ledsC, NUM_LEDS);
  // Button setup
  pinMode(BUTTON_PINA, INPUT_PULLUP);
  pinMode(BUTTON_PINB, INPUT_PULLUP);
  pinMode(BUTTON_PINC, INPUT_PULLUP);

  // Wifi setup
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

  // Enabling OTA
  Serial.println("\nEnabling OTA Feature");
  ArduinoOTA.setPassword("");
  ArduinoOTA.begin();

  // Start API server 
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

/// @brief Call the receiving API in the controlling server to inform the selected numbers for each strip
/// @param countA Selected number for Strip 0
/// @param countB Selected number for Strip 1
/// @param countC Selected number for Strip 2
void sendLedCountToApi(int countA, int countB, int countC)
{
   Serial.println("sendLedCountToApi...");

  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(url2SendResult); // Replace with your API endpoint
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", bearerString); 

//    String payload = "{\"ledA\":" + String(countA) + ",\"ledB\":" + String(countB) + ",\"ledC\":" + String(countC) + "}";
    String payload = "{\"event_type\": \"state_changed\", \"entity_id\": \"custom_sensor.component_transform\", \"event\": { \"entity_id\": \"custom_sensor.component_transform\",\"new_state\": {\"entity_id\": \"custom_sensor.component_transform\", \"state\": \"["+ String(countA)+","+ String(countB)+","+ String(countC) +  "]\"}}}";
    Serial.println(payload);
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

/// @brief check if the button is unpressed to enable other button presses
void CheckButtonUnPress()
{
  // The unpressing of the buttons can be handled independently 
  // of the previous states because it does not change the internal state
  // of the game.
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

/// @brief Check if a button is pressed. Only handle one at a time
void CheckButtonPress()
{

  // Check if I have one button pressed that has not been "unpressed"
  bool isPressed = false;
  for (size_t i = 0; i < 3; i++)
  {
    if (buttonPressed[i])
    {
      isPressed = true;
    }
  }

  // only handle if there is no other button pressed
  if (!isPressed)
  {
    for (size_t i = 0; i < 3; i++)
    {
      // check if it is pressed and has not been already selected yet (no need to handle it)
      if (!buttonPressed[i] && digitalRead(buttonPins[i]) == LOW && !alreadySelected[i])
      {
        // Debounce
        // check again (IMPORTANT: if you do not debounce side effects can be a nightmare)
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
    Serial.println("Seleccionado - ");
    Serial.println(payload);
    sendLedCountToApi(countA, countB, countC);

  }
}

// Main loop
void loop()
{
  ArduinoOTA.handle();

  // if game is shutdown, do not update the LEDs
  if (gameStarted)
  {
    // Check for pressing and unpressing buttons
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
