#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_PIN 2

#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_TIMEOUT  10000

#define MQTT_BROKER    "broker.hivemq.com"
#define MQTT_PORT      1883
#define MQTT_CLIENT_ID "esp32-receiver-rus-nik"

#define TOPIC_TEMPERATURE "iot-course/rus-nik/sensors/temperature"
#define TOPIC_HUMIDITY    "iot-course/rus-nik/sensors/humidity"
#define TOPIC_COMMANDS    "iot-course/rus-nik/commands"
#define TOPIC_LED         "iot-course/rus-nik/actuators/led"
#define TOPIC_STATUS      "iot-course/rus-nik/status"

#define RECONNECT_INTERVAL 5000
#define RECONNECT_MAX      3

unsigned long lastReconnectAttempt = 0;
int reconnectCount = 0;

bool ledState = false;
bool blinking = false;

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

void setLed(bool on) {
    ledState = on;
    digitalWrite(LED_PIN, on ? HIGH : LOW);

    if (mqttClient.connected()) {
        mqttClient.publish(TOPIC_LED, on ? "on" : "off");
    }
}

void blinkLed(int times, int delayMs) {
    blinking = true;
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    blinking = false;
}

void onMessage(char* topic, byte* payload, unsigned int length) {
    char msg[128];
    unsigned int len = length < sizeof(msg) - 1 ? length : sizeof(msg) - 1;
    memcpy(msg, payload, len);
    msg[len] = '\0';

    Serial.print("[MQTT] ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(msg);

    if (strcmp(topic, TOPIC_TEMPERATURE) == 0) {
        float temp = atof(msg);
        if (temp > 26.0) {
            Serial.println("[LED] Temperature > 26 -> ON");
            setLed(true);
        } else if (temp < 20.0) {
            Serial.println("[LED] Temperature < 20 -> OFF");
            setLed(false);
        }
    } else if (strcmp(topic, TOPIC_COMMANDS) == 0) {
        if (strcmp(msg, "manual_read") == 0) {
            Serial.println("Manual trigger received");
            blinkLed(3, 200);
        }
    }
}

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
        mqttClient.subscribe(TOPIC_TEMPERATURE, 1);
        mqttClient.subscribe(TOPIC_HUMIDITY);
        mqttClient.subscribe(TOPIC_COMMANDS);
        mqttClient.publish(TOPIC_STATUS, "receiver online");
        reconnectCount = 0;
        return true;
    }

    Serial.print(" error: ");
    Serial.println(mqttClient.state());
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-receiver start");

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    connectWifi();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(30);
    mqttClient.setCallback(onMessage);
    connectMQTT();
}

void loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    } else {
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
