#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <LoRa.h>

/* WiFi */
const char* ssid = "your wifi name";
const char* password = "your wifi pass";

/* ThingSpeak */
String apiKey = "your api key";

/* Telegram */
String botToken = "your bot token";
String chatID   = "your chat id";

/* Location */
String location = "as your wish";

/* Clients */
WiFiClient client;
WiFiClientSecure secureClient;

/* LoRa Pins */
#define SS   D8
#define RST  D0
#define DIO0 D2

/* Timing */
unsigned long lastUpload = 0;
const long interval = 16000;

/* Data */
float rainMM = 0;
float waterLevel = 0;

/* State control */
int lastState = -1;

/* Telegram Function */
void sendTelegram(String message)
{
  secureClient.setInsecure();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }

  message.replace(" ", "%20");
  message.replace("\n", "%0A");

  String url = "/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;

  Serial.println("Sending Telegram...");
  Serial.println(url);

  if (secureClient.connect("api.telegram.org", 443))
  {
    secureClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                       "Host: api.telegram.org\r\n" +
                       "Connection: close\r\n\r\n");

    Serial.println("Telegram Sent ✅");
  }
  else
  {
    Serial.println("Telegram Failed ❌");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Serial.println("Connecting WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());

  SPI.begin();
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(868E6))
  {
    Serial.println("LoRa Failed!");
    while (1);
  }

  Serial.println("LoRa Receiver Ready");
}

void loop()
{
  /* ===== LoRa Receive ===== */
  int packetSize = LoRa.parsePacket();

  if (packetSize)
  {
    String data = "";

    while (LoRa.available())
    {
      data += (char)LoRa.read();
    }

    Serial.println("Received: " + data);

    float R, D;

    int parsed = sscanf(data.c_str(), "R=%f,D=%f", &R, &D);

    if (parsed != 2)
    {
      Serial.println("Invalid Data");
      return;
    }

    rainMM = R * 10;
    waterLevel = D;

    Serial.println("---- DATA ----");
    Serial.print("Rain: "); Serial.println(rainMM);
    Serial.print("Water Level: "); Serial.println(waterLevel);

    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());

    /* ===== STATE LOGIC ===== */
    int currentState = 0;

    bool floodHigh = (waterLevel > 0 && waterLevel <= 30);
    bool waterMid  = (waterLevel > 30 && waterLevel <= 60);
    bool waterLow  = (waterLevel > 60);
    bool heavyRain = (rainMM > 50);

    if (floodHigh && heavyRain)
      currentState = 4;

    else if (floodHigh)
      currentState = 3;

    else if (waterMid)
      currentState = 2;

    else if (heavyRain)
      currentState = 1;

    else
      currentState = 0;

    /* ===== TELEGRAM ===== */
    if (currentState != lastState)
    {
      lastState = currentState;

      String msg = "";

      if (currentState == 4)
        msg = "🚨 FLOOD ALERT 🚨\nGo to safe place immediately!";
        
      else if (currentState == 3)
        msg = "🚨 High Flood Alert\nWater level is very high!";
        
      else if (currentState == 2)
        msg = "⚠ Water Increasing\nBe alert!";
        
      else if (currentState == 1)
        msg = "🌧 Heavy Rain\nChance of flood";
        
      else
        msg = "✅ Water at base level (Normal)";

      msg = "Location: " + location + "\n\n" + msg;

      sendTelegram(msg);
    }
  }

  /* ===== ThingSpeak ===== */
  if (millis() - lastUpload > interval)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;

      String url = "http://api.thingspeak.com/update?api_key=" + apiKey +
                   "&field1=" + String(rainMM) +
                   "&field2=" + String(waterLevel);

      http.begin(client, url);
      int httpCode = http.GET();

      Serial.print("ThingSpeak Response: ");
      Serial.println(httpCode);

      http.end();
    }

    lastUpload = millis();
  }
}