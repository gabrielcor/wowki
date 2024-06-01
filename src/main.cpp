#include <FastLED.h>
#include <WiFi.h>
#include <HTTPClient.h>
#define NUM_LEDS 20 
#define DATA_PINA 17 // Connect to the data wires on the pixel strips
#define DATA_PINB 19
#define DATA_PINC 16
#define BUTTON_PINA  5
#define BUTTON_PINB 25
#define BUTTON_PINC 24

bool gameStarted = true;
bool buttonPressed = false;

CRGB ledsA[NUM_LEDS]; // sets number of pixels that will light on each strip.
CRGB ledsB[NUM_LEDS];
CRGB ledsC[NUM_LEDS];


const CRGB coo = CRGB::Green;

const char* url2SendResult = "https://eovunmo8a5u8h34.m.pipedream.net";
/*
const char* ssid = "Wokwi-GUEST";
const char* password = "";
*/
const char* ssid = "blackcrow_01";
const char* password = "8001017170";
// LED control variables
#define CANT_STRIPS 3;
int ledIndexes[3];
CRGB ledColors[3];

bool increasing[3];
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 50; 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  for (int i = 0; i < 3; i++)
  {
    ledIndexes[i]=0;
    increasing[i]=true;
  }
  ledColors[0]=CRGB::Green;
  ledColors[1]=CRGB::Blue;
  ledColors[2]=CRGB::Orange;

  
  Serial.println("Hello, ESP32!");
  FastLED.addLeds<WS2812B, DATA_PINA, GRB>(ledsA, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PINB, GRB>(ledsB, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PINC, GRB>(ledsC, NUM_LEDS);
  pinMode(BUTTON_PINA, INPUT_PULLUP);

    WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

}



void updateLeds(CRGB *led2Update, int stripNumber) {
  int paintTo=0;
  if (increasing[stripNumber]) {
        
    ledIndexes[stripNumber]++;
    if (ledIndexes[stripNumber] >= NUM_LEDS) {
      increasing[stripNumber] = false;
    }
  } else {

    
    ledIndexes[stripNumber]--;
    if (ledIndexes[stripNumber] <= 0) {
      increasing[stripNumber] = true;
    }
  }
    paintTo = ledIndexes[stripNumber];  
    for (size_t i = 0; i < NUM_LEDS; i++)
    {
        if (i<paintTo)
          led2Update[i]= ledColors[stripNumber];  
        else
           led2Update[i]=CRGB::Black;
    }
  /*
  
  Serial.print("ledIndex Direction, Strip, Index:");
  if (increasing[stripNumber])
    Serial.print("increasing,");
  else
    Serial.print("decreasing,");
  Serial.print(stripNumber);
  Serial.print(",");
  Serial.println(ledIndexes[stripNumber]);
  */
  
}


void sendLedCountToApi(int countA, int countB, int countC) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url2SendResult);  // Replace with your API endpoint
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"ledA\":" + String(countA) + ",\"ledB\":" + String(countB) + ",\"ledC\":"+ String(countC)+ "}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void loop() {


  if (gameStarted) {
    // Check button press A
    if (digitalRead(BUTTON_PINA) == LOW)  {
     
      if (!increasing) // si estamos decrementando es uno menos
          { ledIndexes[0]--;}
      // sendLedCountToApi(ledIndexes[0],ledIndexes[1],ledIndexes[2]);
      gameStarted = false;  // Stop the game after sending the count
    } /* else if (digitalRead(BUTTON_PINA) == HIGH) {
      buttonPressed = false;
    } */

    // Update LED sequence
    
    if ((millis() - lastUpdate) > updateInterval) {
      lastUpdate = millis();
      updateLeds(ledsA,0);
       updateLeds(ledsB,1);
       updateLeds(ledsC,2);
      FastLED.show();
    }
  }
  else
  {
      if (digitalRead(BUTTON_PINA) == LOW ) {
  
      gameStarted = true;  // Restart the game
   
    }

  }

  }



