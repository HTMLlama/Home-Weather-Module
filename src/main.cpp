#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <keys.h>
#include <FastLED.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

#define DHT_PIN 14
#define DHT_TYPE DHT11

const char* LB = "-------------------------------";

WiFiClient wiFiClient;
PubSubClient pubSubClient(wiFiClient);
long lastMessage = 0;
char msg[100];
int value = 0;
float lastTempValue = 0.00;
float lastHumValue = 0.00;
bool isFirstLoad = true;

DHT dht(DHT_PIN, DHT_TYPE);
char dtostrfChars[8];

void callback(char* topic, byte* message, unsigned int lenght) {
  String messageTemp;
  for (size_t i = 0; i < lenght; i++) {
    messageTemp += (char) message[i];
  }

  Serial.print("Message on Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(messageTemp);
}

void setupWifi() {
  delay(12);
  Serial.println(LB);
  Serial.print("Connecting to: ");
  Serial.println(SSID);

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED){
    delay(420);
    Serial.print(">");
  }

  Serial.println();
  Serial.println("Connected to WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void setupPubSub() {
  pubSubClient.setServer(MQTT_SERVER, 1883);
  pubSubClient.setCallback(callback);
}

void reconnectToPubSub() {
  while(!pubSubClient.connected()) {
    Serial.println("MQTT Connecting...");
    if (pubSubClient.connect("KitchenClimate")) {
      Serial.println("Connected to MQTT!");
      pubSubClient.subscribe("esp32/weather/#");
    } else {
      Serial.println("Failed to connect to MQTT");
      Serial.print("State: ");
      Serial.println(pubSubClient.state());
      Serial.println("Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  setupWifi();
  setupPubSub();

  dht.begin();

  delay(1500);
}

void loop() {

  if (!pubSubClient.connected()){
    reconnectToPubSub();
  }
  pubSubClient.loop();

  EVERY_N_SECONDS(15) {
    float h = dht.readHumidity();
    // float c = dht.readTemperature();
    float f = dht.readTemperature(true);
    if (isnan(f) || isnan(h)) {
        Serial.println("Failed to read from DHT sensor!");
        pubSubClient.publish("weather/e", "Unknown Error");
        return;
    }
    float hif = dht.computeHeatIndex(f, h);
    // float hic = dht.computeHeatIndex(t, h, false);

    if (isFirstLoad || !(f - lastTempValue >= 7)) {
      lastTempValue = f;
      delay(200);
      pubSubClient.publish("kitchen/weather/f", dtostrf(f, 4, 2, dtostrfChars));
      delay(200); 
      if (isFirstLoad || !(f - lastHumValue >= 7)) {
        pubSubClient.publish("kitchen/weather/h", dtostrf(h, 4, 2, dtostrfChars));
        delay(200); 
        pubSubClient.publish("kitchen/weather/hif", dtostrf(hif, 4, 2, dtostrfChars));
      }
      isFirstLoad = false;
    }
  }
  
}