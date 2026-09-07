#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHT_PIN 4

#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_TIMEOUT  10000

#define MQTT_BROKER    "broker.hivemq.com"
#define MQTT_PORT      1883
#define MQTT_CLIENT_ID "esp32-sender-id"
#define TOPIC_SENSORS  "iot-course/rus-nik/sensors"

#define RECONNECT_INTERVAL  5000
#define PUBLISH_INTERVAL    10000

unsigned long lastPublish = 0;
unsigned long lastReconnectAttempt = 0;

DHT dht(DHT_PIN, DHT22);

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

bool connectWifi() {
    Serial.print("[Wi-Fi] Connecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT) {
            Serial.println(" timeout!");
            return false;
        }
        delay(300);
        Serial.print(".");
    }

    Serial.println(" OK");
    Serial.print("[Wi-Fi] IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

bool connectMQTT() {
    Serial.print("[MQTT] Connecting to ");
    Serial.print(MQTT_BROKER);
    Serial.print(" ...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
        Serial.println(" OK");
        return true;
    }

    Serial.print(" error: ");
    Serial.println(mqttClient.state());
    return false;
}

void publishData(float temperature, float humidity) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Not Connected —> Skip");
        return;
    }

    char payload[80];
    snprintf(payload, sizeof(payload),
        "{\"temperature\":%.1f,\"humidity\":%.1f}", temperature, humidity);

    Serial.print("[MQTT] Publishing: ");
    Serial.println(payload);

    bool isOk = mqttClient.publish(TOPIC_SENSORS, payload);
    Serial.println(isOk ? "[MQTT] OK" : "[MQTT] Error in data publishing");
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-sender start");

    dht.begin();
    connectWifi();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(30);
    connectMQTT();
}

void loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();

        unsigned long now = millis();
        if ((now - lastPublish) > PUBLISH_INTERVAL) {
            lastPublish = now;
            publishData(dht.readTemperature(), dht.readHumidity());
        }
    } else {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            Serial.println("[MQTT] Connection lost —> reconnect...");
            connectMQTT();
        }
    }
}
