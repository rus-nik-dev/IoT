#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "network.h"

void connectWifi() {
  Serial.print("[Wi-Fi] Loading...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT) {
      Serial.println(" timeout!");
      return;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println(" OK");
  Serial.print("[Wi-Fi] IP: ");
  Serial.println(WiFi.localIP());
}

void sendData(float temperature, float humidity, float lux) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Wi-Fi not connected — skip");
    return;
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"temperature\":%.1f,\"humidity\":%.1f,\"lux\":%.1f}",
           temperature, humidity, lux);

  Serial.print("[HTTP] Sending: ");
  Serial.println(payload);

  int httpCode = http.POST((uint8_t *)payload, strlen(payload));

  if (httpCode == 200) {
    Serial.println("[HTTP] OK");
  } else {
    Serial.print("[HTTP] Error: ");
    Serial.println(httpCode);
  }

  http.end();
}
