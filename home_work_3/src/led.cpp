#include <Arduino.h>
#include "config.h"
#include "led.h"

void ledBegin() {
  pinMode(LED_PIN, OUTPUT);
}

void controlLed(float currentLux) {
  digitalWrite(LED_PIN, currentLux < LUX_THRESHOLD ? HIGH : LOW);
}
