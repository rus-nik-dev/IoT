#include <Arduino.h>

struct SensorData {
  float temperature;
  float humidity;
  unsigned long timestamp;
};

SensorData sensor;

SensorData generateReading() {
  SensorData data;
  data.temperature = 15.0 + (random(0, 1501) / 100.0);
  data.humidity = 30.0 + (random(0, 3501) / 100.0);
  data.timestamp = millis();
  return data;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
}

void printSensorData(SensorData sensor) {
  Serial.print("Temperature: ");
  Serial.print(sensor.temperature, 2);

  Serial.print(" C | Humidity: ");
  Serial.print(sensor.humidity, 2);

  Serial.print(" % | Timestamp: ");
  Serial.println(sensor.timestamp);
}

void printMemoryState() {
  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap() / 1024.0, 2);
  Serial.println(" KB");
}

void loop() {

  for(uint8_t i = 0; i < 3; i++) {
    sensor = generateReading();
    printSensorData(sensor);
    delay(20000);
  }

  printMemoryState();
}
