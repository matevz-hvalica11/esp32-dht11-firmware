#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define WIFI_SSID     "Laura"
#define WIFI_PASS     "zahvala1"
#define MQTT_BROKER   "192.168.178.62"
#define MQTT_PORT     1883
#define MQTT_TOPIC    "ism/dht22"
#define MQTT_CLIENT   "esp32-dht11"

#define DHT_PIN       4
#define DHT_TYPE      DHT11
#define INTERVAL_MS   10000

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastSend = 0;

void connectWiFi() {
  Serial.print("WiFi connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK: " + WiFi.localIP().toString());
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("MQTT connecting...");
    if (mqtt.connect(MQTT_CLIENT)) {
      Serial.println("OK");
    } else {
      Serial.printf("failed (rc=%d), retry in 3s\n", mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  connectWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
}

void loop() {
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  if (millis() - lastSend >= INTERVAL_MS) {
    lastSend = millis();

    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("DHT11 read error");
      return;
    }

    StaticJsonDocument<128> doc;
    doc["temperature"] = temp;
    doc["humidity"]    = hum;
    doc["device"]      = MQTT_CLIENT;

    char payload[128];
    serializeJson(doc, payload);

    mqtt.publish(MQTT_TOPIC, payload);
    Serial.printf("Sent: %s\n", payload);
  }
}