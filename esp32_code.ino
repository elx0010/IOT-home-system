#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"
#include <DHT22.h>
#define DHT_PIN 4



const char* DEVICE_ID = "esp32-room-node";
const int   LED_PIN   = 2;



WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
DHT22 dht22(DHT_PIN);
unsigned long lastPublish = 0;



void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("[MQTT] ");
  Serial.print(topic);
  Serial.print(" => ");
  Serial.println(msg);

  if (String(topic) == "home/esp32/led") {
    msg.toUpperCase();
    digitalWrite(LED_PIN, (msg == "ON") ? HIGH : LOW);
  }
}


void connectWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WiFi] Connected. IP:");
  Serial.println(WiFi.localIP());
}


void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting... ");
    if (mqttClient.connect(DEVICE_ID)) {
      Serial.println("connected");
      mqttClient.subscribe("home/esp32/led");
      mqttClient.publish("home/esp32/status", "online");
    } else {
      Serial.print("failed rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}


void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  delay(1000);


  Serial.println("[DHT22] Sensor initialized");

  connectWiFi();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}


void loop() {
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublish > 5000) {
    lastPublish = now;
  float temperature = dht22.getTemperature();
  float temperatureF = (temperature * 1.8) + 32.0;
  float humidity = dht22.getHumidity();
  if (dht22.getLastError() != dht22.OK) {
    Serial.print("last error: ");
    Serial.println(dht22.getLastError());
  }

Serial.print("Temp: ");
Serial.print(temperature);
Serial.println("°C");

Serial.print("Temp: ");
Serial.print(temperatureF);
Serial.println("°F");

Serial.print("Humidity: ");
Serial.print(humidity);
Serial.println("%");

  char tempBuf[8];
  char humBuf[8];
  char tempFBuf[8];
  snprintf(tempFBuf, sizeof(tempFBuf), "%.1f", temperatureF);
  snprintf(tempBuf, sizeof(tempBuf), "%.1f", temperature);
  snprintf(humBuf, sizeof(humBuf), "%.1f", humidity);
  
  mqttClient.publish("home/esp32/temperature", tempBuf);
  mqttClient.publish("home/esp32/humidity", humBuf);
  mqttClient.publish("home/esp32/temperatureF", tempFBuf);
  
  }
}
