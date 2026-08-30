#include <Arduino.h>
#include "DHT.h"
#include "config.h"
#include "sensors.h"

static DHT dht(DHT_PIN, DHT22);

void sensorsBegin() {
  dht.begin();
}

float readLux() {
  int adcValue = analogRead(LDR_PIN);
  float voltage = adcValue / 4096.0f * 3.3f;
  float resistance = 10000.0f * voltage / (3.3f - voltage);
  return pow(50.0f * 1000.0f * pow(10, 0.7f) / resistance, (1.0f / 0.7f));
}

bool readDHT(float &temperature, float &humidity) {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  return !isnan(humidity) && !isnan(temperature);
}
