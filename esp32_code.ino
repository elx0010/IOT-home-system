#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

const char* DEVICE_ID = "esp32-room-node";
const int   LED_PIN   = 2;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
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

    int fakeTemp = random(60, 81);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", fakeTemp);

    mqttClient.publish("home/esp32/temperature", buf);
  }
}
