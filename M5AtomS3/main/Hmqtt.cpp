#include "M5AtomS3.h"
#include "Hmqtt.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>

const char* Hmqtt::ssid        = "Buffalo-G-2598";
const char* Hmqtt::password    = "x3fgskfwxeenf";
const char* Hmqtt::mqtt_server = "10.0.0.3";

WiFiClient Hmqtt::espClient;
PubSubClient Hmqtt::client(Hmqtt::espClient);

void Hmqtt::setupWifi()
{
    Serial.printf("Connecting to %s", ssid);

    WiFi.mode(WIFI_STA);                        // Set the mode to WiFi station mode.
    WiFi.begin(Hmqtt::ssid, Hmqtt::password);   // Start Wifi connection.

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nSuccess\n");
}

void Hmqtt::reConnect()
{
    while (! Hmqtt::client.connected()) {
        Serial.print("Attempting MQTT connection...");      // Create a random client ID.

        String clientId = "M5Stack-";
        clientId += String(random(0xffff), HEX);            // Attempt to connect.

        if ( Hmqtt::client.connect(clientId.c_str()) ) {
            Serial.println("connected");                    // Once connected, publish an announcement to the topic.
            // Hmqtt::client.subscribe(topic);
        } else {
            Serial.print("failed, rc=");
            Serial.print(Hmqtt::client.state());
            Serial.println("try again in 5 seconds");
            delay(5000);
        }
    }
}

void Hmqtt::setServer()
{
  Hmqtt::client.setServer(Hmqtt::mqtt_server, 1883);
}

void Hmqtt::setCallback(MQTT_CALLBACK_SIGNATURE)
{
  Hmqtt::client.setCallback(callback);
}

void Hmqtt::update()
{
  if (! Hmqtt::client.connected()) {
    Hmqtt::reConnect();
  }
  Hmqtt::client.loop();
}