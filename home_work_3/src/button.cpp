#include <Arduino.h>
#include "config.h"
#include "button.h"

static volatile bool buttonPressed = false;

static void IRAM_ATTR onButtonPress() {
  buttonPressed = true;
}

void buttonBegin() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, onButtonPress, FALLING);
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
