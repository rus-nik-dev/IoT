#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHT_PIN 4
#define BUTTON_PIN 5
#define DEBOUNCE_INTERVAL 50

#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_TIMEOUT  10000

#define MQTT_BROKER    "broker.hivemq.com"
#define MQTT_PORT      1883
#define MQTT_CLIENT_ID "esp32-sender-rus-nik"

#define TOPIC_TEMPERATURE "iot-course/rus-nik/sensors/temperature"
#define TOPIC_HUMIDITY    "iot-course/rus-nik/sensors/humidity"
#define TOPIC_COMMANDS    "iot-course/rus-nik/commands"
#define TOPIC_STATUS      "iot-course/rus-nik/status"

#define RECONNECT_INTERVAL  5000
#define RECONNECT_MAX       3
#define PUBLISH_INTERVAL    10000

unsigned long lastPublish = 0;
unsigned long lastReconnectAttempt = 0;
uint8_t reconnectCount = 0;

static volatile bool buttonPressed = false;

static void IRAM_ATTR onButtonPress() {
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
    Serial.print("...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
        Serial.println(" OK");
        mqttClient.publish(TOPIC_STATUS, "sender online");
        reconnectCount = 0;
        return true;
    }

    Serial.print(" error: ");
    Serial.println(mqttClient.state());
    return false;
}

void publishData(const char* topic, const char* payload) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Not connected -> skip");
        return;
    }

    Serial.print("[MQTT] ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload);

    bool ok = mqttClient.publish(topic, payload);
    Serial.println(ok ? "[MQTT] OK" : "[MQTT] Error");
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-sender start");

    dht.begin();
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(BUTTON_PIN, onButtonPress, FALLING);
    connectWifi();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(30);
    connectMQTT();
}

void loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();

        if (isButtonPressedWithDebounce()) {
            publishData(TOPIC_COMMANDS, "manual_read");
        }

        unsigned long now = millis();
        if (now - lastPublish > PUBLISH_INTERVAL) {
            lastPublish = now;

            float t = dht.readTemperature();
            float h = dht.readHumidity();

            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", t);
            publishData(TOPIC_TEMPERATURE, buf);

            snprintf(buf, sizeof(buf), "%.1f", h);
            publishData(TOPIC_HUMIDITY, buf);
        }
    } else {
        if (isButtonPressedWithDebounce()) {
            Serial.println("[Button] pressed (offline)");
        }

        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;

            if (reconnectCount < RECONNECT_MAX) {
                reconnectCount++;
                Serial.print("[MQTT] Reconnect attempt ");
                Serial.print(reconnectCount);
                Serial.print("/");
                Serial.println(RECONNECT_MAX);
                connectMQTT();
            } else {
                Serial.println("[MQTT] Max reconnect attempts reached");
            }
        }
    }
}
