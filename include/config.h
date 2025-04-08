#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>  
#include <Preferences.h>
#include <ArduinoJson.h>

// Default Wi-Fi and MQTT settings
const char* defaultSSID = "ESP32-AP";
const char* defaultPassword = "12345678";
const int gpioPin = 2;
bool uartEnabled = true;  // globally used

// Objects initialization
WebServer server(80);
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

// Variables for settings
String wifiSSID = "";
String wifiPassword = "";
String mqttServer = "";
int mqttPort = 8883; // Port for TLS connection
String mqttUser = "";
String mqttPass = "";

// Initialize Preferences for saving settings
Preferences preferences;

// Initialize NTP
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 10800, 60000); // Time in UA

#endif