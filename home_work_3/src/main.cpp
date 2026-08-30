#include <Arduino.h>
#include "DHT.h"

#define LED_PIN 2
#define DHT_PIN 4
#define LDR_PIN 34
#define BUTTON_PIN 5

#define LUX_THRESHOLD 700
#define SENSOR_INTERVAL 500
#define DEBOUNCE_INTERVAL 50

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

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, onButtonPress, FALLING);
  dht.begin();
}

void loop() {
  if (millis() - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = millis();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    float lux = adcToLux(analogRead(LDR_PIN));

    controlLed(lux);
  }

  if(isButtonPressedWithDebounce()) {
    Serial.println("button pressed");
  }
}
