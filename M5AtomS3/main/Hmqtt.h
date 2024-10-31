#ifndef _HMQTT_H_
#define _HMQTT_H_
#include <WiFi.h>
#include <PubSubClient.h>

struct Hmqtt
{
    static const char* ssid;
    static const char* password;
    static const char* mqtt_server;

    static WiFiClient   espClient;
    static PubSubClient client;

    static void setupWifi();
    static void reConnect();
    static void setServer();
    static void update();
    static void setCallback(MQTT_CALLBACK_SIGNATURE);
    static void callback(char* topic, byte* payload, unsigned int length);
    static void init();
};

#endif
