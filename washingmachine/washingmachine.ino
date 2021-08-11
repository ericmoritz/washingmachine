/*
 * Washing Machine -> MQTT bridge
 *
 * This reads two LIC3DH sensors and publishes x,y,z acceleration values
 * to MQTT topics to be processed elsewhere
 */

#include "config.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>


/************************* mqtt Setup *********************************/

#define MQTT_SERVER    "mqtt.home"
#define MQTT_PORT      1883
#define MQTT_CLIENT_ID "washer"
#define STATUS_TOPIC "/machines/washer/status"
#define SENSOR_0_TOPIC "/machines/washer/lis/0"
#define SENSOR_1_TOPIC "/machines/washer/lis/1"

WiFiClient wifiClient;

PubSubClient pubSubClient(wifiClient);

// I2C
Adafruit_LIS3DH lis0 = Adafruit_LIS3DH();
Adafruit_LIS3DH lis1 = Adafruit_LIS3DH();

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup mqtt
  pubSubClient.setServer(MQTT_SERVER, MQTT_PORT);
  
  // Setup lis0
  if (! lis0.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start accel");
    while (1) yield();
  }
  Serial.println("LIS3DH 0 found!");

  if (! lis1.begin(0x19)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start accel");
    while (1) yield();
  }
  Serial.println("LIS3DH 1 found!");


  lis0.setRange(LIS3DH_RANGE_2_G);   // 2, 4, 8 or 16 G!
  lis1.setRange(LIS3DH_RANGE_2_G);   // 2, 4, 8 or 16 G!

  Serial.print("Range = "); Serial.print(2 << lis0.getRange());
  Serial.println("G");
}


void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  sensors_event_t event;
  
  lis0.getEvent(&event);
  publishEvent(SENSOR_0_TOPIC, &event);

  lis1.getEvent(&event);
  publishEvent(SENSOR_1_TOPIC, &event);
  
  delay(200);
}

void publishEvent(char *topic, sensors_event_t *event) {
  char buffer[256];
  StaticJsonDocument<256> doc;
  
  doc["x"] = event->acceleration.x;
  doc["y"] = event->acceleration.y;
  doc["z"] = event->acceleration.z;
  
  size_t n = serializeJson(doc, buffer);
  pubSubClient.publish(topic, buffer, n);
  Serial.print(topic);
  Serial.print(": ");
  Serial.print(buffer);
  Serial.println();
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  if (pubSubClient.connected()) {
    return;  
  }
// Loop until we're reconnected
  while (!pubSubClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (pubSubClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      pubSubClient.publish(STATUS_TOPIC, "hello");
    } else {
      Serial.print("failed, rc=");

      Serial.print(pubSubClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
