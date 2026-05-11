#include <SPI.h>
#include <LoRa.h>

/* ---- Pins ---- */
#define RAINPIN A0

#define TRIGPIN 5
#define ECHOPIN 6

/* LoRa Pins */
#define SS   10
#define RST  9
#define DIO0 2

void setup() {
  Serial.begin(9600);

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(868E6)) {
    Serial.println("LoRa Failed!");
    while (1);
  }

  Serial.println("Sender Ready");
}

/* ---- Ultrasonic Function ---- */
float getDistance() {

  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(5);

  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(15);
  digitalWrite(TRIGPIN, LOW);

  long duration = pulseIn(ECHOPIN, HIGH, 30000);

  if (duration == 0) return 0;

  float distance = duration * 0.034 / 2;

  if (distance > 400 || distance < 20) {
    return 0;
  }

  return distance;
}

void loop() {

  int rawRain = analogRead(RAINPIN);
  int rainLevel = map(rawRain, 1023, 0, 0, 10);

  float distance = getDistance();

  Serial.println("------ DATA ------");
  Serial.print("Rain Level: ");
  Serial.println(rainLevel);

  Serial.print("Distance: ");
  Serial.println(distance);

  /* ✅ FIXED FORMAT */
  String msg = "R=" + String(rainLevel) +
               ",D=" + String(distance,1);

  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  Serial.println("Data Sent\n");

  delay(16000);
}