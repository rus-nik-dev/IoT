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
#define MQTT_CLIENT_ID "esp32-sender-id"
#define TOPIC_SENSORS  "iot-course/rus-nik/sensors"
#define TOPIC_COMMANDS  "iot-course/rus-nik/commands"

#define RECONNECT_INTERVAL  5000
#define PUBLISH_INTERVAL    10000

unsigned long lastPublish = 0;
unsigned long lastReconnectAttempt = 0;

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
    Serial.print(" ...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
        Serial.println(" OK");
        return true;
    }

    Serial.print(" error: ");
    Serial.println(mqttClient.state());
    return false;
}

const char* buildSensorPayload(float temperature, float humidity) {
    static char buf[80];
    snprintf(buf, sizeof(buf), "{\"temperature\":%.1f,\"humidity\":%.1f}", temperature, humidity);
    return buf;
}

void publishData(const char* topic, const char* payload) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Not Connected —> Skip");
        return;
    }

    Serial.print("[MQTT] Publishing to ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload);

    bool isOk = mqttClient.publish(topic, payload);
    Serial.println(isOk ? "[MQTT] OK" : "[MQTT] Error in data publishing");
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
        if ((now - lastPublish) > PUBLISH_INTERVAL) {
            lastPublish = now;

            const char* payload = buildSensorPayload(dht.readTemperature(), dht.readHumidity());
            publishData(TOPIC_SENSORS, payload);
        }
    } else {
        if (isButtonPressedWithDebounce()) {
          Serial.println("[Button] pressed.");
        }

        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            Serial.println("[MQTT] Connection lost —> reconnect...");
            connectMQTT();
        }
    }
}
