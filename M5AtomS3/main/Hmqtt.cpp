#include "M5AtomS3.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include "Hmqtt.h"

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

    while ( WiFi.status() != WL_CONNECTED ) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\r\nSuccess\r\n");
}

void Hmqtt::reConnect()
{
    while (! Hmqtt::client.connected() ) {
        Serial.print("Attempting MQTT connection...");      // Create a random client ID.

        String clientId = "M5Stack-";
        clientId += String(random(0xffff), HEX);            // Attempt to connect.

        if ( Hmqtt::client.connect(clientId.c_str()) ) {
            Serial.println("connected");                    // Once connected, publish an announcement to the topic.
            // Hmqtt::client.subscribe(topic);
        } else {
            Serial.print("failed, rc=");
            Serial.print( Hmqtt::client.state() );
            Serial.println("try again in 5 seconds");
            delay(5000);
        }
    }
}

void Hmqtt::callback(char* topic, byte* payload, unsigned int length)
{
    Serila.printf("message detected!\r\n");
}

void Hmqtt::init()
{
  Hmqtt::setupWifi();
  Hmqtt::client.setServer(Hmqtt::mqtt_server, 1883);
  Hmqtt:client.setCallback(Hmqtt:callback);
}

void Hmqtt::update()
{
    if (! Hmqtt::client.connected()) {
        Hmqtt::reConnect();
    }
    Hmqtt::client.loop();
}
