#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "led.h"
#include "button.h"
#include "network.h"

static Mode currentMode = MONITORING;
static unsigned long lastSensorRead = 0;
static unsigned long lastSendData = 0;

void checkMode() {
  if (isButtonPressedWithDebounce()) {
    currentMode = (currentMode == MONITORING) ? SILENT : MONITORING;
    Serial.print("[Mode] Switched to: ");
    Serial.println((currentMode == MONITORING) ? "Monitoring" : "Silent");
  }
}

void monitoringMode() {
  float humidity, temperature;
  float lux = readLux();

  if (!readDHT(temperature, humidity)) {
    if (millis() - lastSensorRead > SENSOR_INTERVAL) {
      lastSensorRead = millis();
      Serial.println("[Sensors] Error: failed to read from DHT22");
    }
    return;
  }

  if (millis() - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = millis();
    Serial.printf("[Sensors] Temperature: %.1f°C, Humidity: %.1f%%, Lux: %.1f\n", temperature, humidity, lux);
  }

  if (millis() - lastSendData > SEND_DATA_INTERVAL) {
    lastSendData = millis();
    sendData(temperature, humidity, lux);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  ledBegin();
  buttonBegin();
  sensorsBegin();
  connectWifi();
}

void loop() {
  checkMode();
  if (currentMode == MONITORING) {
    monitoringMode();
  }
  controlLed(readLux());
}
