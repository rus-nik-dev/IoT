#include <Arduino.h>

#define LED 4

void signal(char symbol);
void letter(char symbol);

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  letter('S');  // ...
  letter('O');  // ---
  letter('S');  // ...

  Serial.println();
  delay(2000);
}

void letter(char symbol) {
  if (symbol == 'S') {
    for (int i = 0; i < 3; i++) {
      signal('.');
    }
  }

  if (symbol == 'O') {
    for (int i = 0; i < 3; i++) {
      signal('-');
    }
  }

  delay(400);
}

void signal(char symbol) {
  digitalWrite(LED, HIGH);
  Serial.print(symbol);

  if (symbol == '.') {
    delay(200);
  } else {
    delay(600);
  }

  digitalWrite(LED, LOW);
  delay(200);
}
