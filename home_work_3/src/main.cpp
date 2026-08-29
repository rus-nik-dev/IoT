#include <Arduino.h>
#include "DHT.h"

#define LED_PIN 2
#define LDR_PIN 34

DHT dht(4, DHT22);

float adcToLux(int adcValue) {
  float voltage = adcValue / 4096.0f * 3.3f;
  float resistance = 10000.0f * voltage / (3.3f - voltage);
  return pow(50.0f * 1e3 * pow(10, 0.7f) / resistance, (1.0f / 0.7f));
}

void controlLed(float currentLux, int targetLux) {
  if (currentLux < targetLux) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  dht.begin();
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float lux = adcToLux(analogRead(LDR_PIN));

  controlLed(lux, 1e3);

  delay(500);
}
