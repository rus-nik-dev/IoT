#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"

#define LED_PIN 2
#define DHT_PIN 4
#define LDR_PIN 34
#define BUTTON_PIN 5

#define LUX_THRESHOLD 700
#define SENSOR_INTERVAL 2000
#define DEBOUNCE_INTERVAL 50

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_TIMEOUT  10000
#define SERVER_URL "http://httpbun.com/post"

DHT dht(DHT_PIN, DHT22);

unsigned long lastSensorRead = 0;
volatile bool buttonPressed = false;

// some magic...
float adcToLux(int adcValue) {
  float voltage = adcValue / 4096.0f * 3.3f;
  float resistance = 10000.0f * voltage / (3.3f - voltage);
  return pow(50.0f * 1000.0f * pow(10, 0.7f) / resistance, (1.0f / 0.7f));
}

void controlLed(float currentLux) {
  if (currentLux < LUX_THRESHOLD) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void IRAM_ATTR onButtonPress() {
  buttonPressed = true;
}

bool isButtonPressedWithDebounce() {
  static bool waitingForRelease = false;
  static unsigned long releaseStarted = 0;

  if (waitingForRelease) {
    buttonPressed = false;

    if (digitalRead(BUTTON_PIN) == HIGH) {
      if (releaseStarted == 0) {
        releaseStarted = millis();
      }

      if (millis() - releaseStarted >= DEBOUNCE_INTERVAL) {
        waitingForRelease = false;
        releaseStarted = 0;
      }
    } else {
      releaseStarted = 0;
    }

    return false;
  }

  if (buttonPressed) {
    buttonPressed = false;

    if (digitalRead(BUTTON_PIN) == LOW) {
      waitingForRelease = true;
      return true;
    }
  }

  return false;
}

void connectWifi() {
    Serial.print("[Wi-Fi] Loading...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT) {
            Serial.println(" timeout!");
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

    int httpCode = http.POST((uint8_t *) payload, strlen(payload));

    if (httpCode == 200) {
        Serial.println("[HTTP] OK");
    } else {
        Serial.print("[HTTP] Error: ");
        Serial.println(httpCode);
    }

    http.end();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, onButtonPress, FALLING);
  dht.begin();

  connectWifi();
}

void loop() {
  float lux = adcToLux(analogRead(LDR_PIN));
  controlLed(lux);

  if (millis() - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = millis();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    sendData(temperature, humidity, lux);
  }

  if(isButtonPressedWithDebounce()) {
    Serial.println("button pressed");
  }
}
