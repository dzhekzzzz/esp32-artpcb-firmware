#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define GPIO_PIN 2
#define FW_VERSION "1.0.0"

Preferences preferences;

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * 2); // CET

String wifiSSID, wifiPassword, mqttServer, mqttUser, mqttPass;
int mqttPort;
bool uartEnabled = true;

unsigned long lastMsg = 0;
int messageID = 1001;

void saveConfig() {
  preferences.begin("config", false);
  preferences.putString("ssid", wifiSSID);
  preferences.putString("pass", wifiPassword);
  preferences.putString("mqttServer", mqttServer);
  preferences.putInt("mqttPort", mqttPort);
  preferences.putString("mqttUser", mqttUser);
  preferences.putString("mqttPass", mqttPass);
  preferences.end();
}

void loadConfig() {
  preferences.begin("config", true);
  wifiSSID = preferences.getString("ssid", "");
  wifiPassword = preferences.getString("pass", "");
  mqttServer = preferences.getString("mqttServer", "");
  mqttPort = preferences.getInt("mqttPort", 1883);
  mqttUser = preferences.getString("mqttUser", "");
  mqttPass = preferences.getString("mqttPass", "");
  preferences.end();
}

void handleRoot() {
  String html = R"rawliteral(
    <html><body><h1>ESP32 Setup</h1>
    <form action="/save">
      SSID: <input name="ssid"><br>
      Password: <input name="pass"><br>
      MQTT Server: <input name="mqtt"><br>
      MQTT Port: <input name="port" type="number"><br>
      MQTT User: <input name="user"><br>
      MQTT Pass: <input name="mpass"><br>
      <input type="submit" value="Save">
    </form></body></html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleSave() {
  wifiSSID = server.arg("ssid");
  wifiPassword = server.arg("pass");
  mqttServer = server.arg("mqtt");
  mqttPort = server.arg("port").toInt();
  mqttUser = server.arg("user");
  mqttPass = server.arg("mpass");
  saveConfig();
  server.send(200, "text/html", "Saved. Rebooting...");
  delay(1000);
  ESP.restart();
}

void setupWebServer() {
  WiFi.softAP("ESP32_Config", "12345678");
  IPAddress IP = WiFi.softAPIP();
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}

void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqttUser.c_str(), mqttPass.c_str())) {
      client.subscribe("uart/control");
      client.subscribe("gpio/control");
    } else {
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) message += (char)payload[i];
  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, message);
  if (err) return;
  if (String(topic) == "uart/control") {
    uartEnabled = doc["state"];
  } else if (String(topic) == "gpio/control") {
    digitalWrite(GPIO_PIN, doc["state"] ? HIGH : LOW);
  }
}

// float readInternalTemperature() {
//   // return (temprature_sens_read() - 32) / 1.8;
// }

void publishStatus() {
  StaticJsonDocument<256> doc;
  char macStr[18];
  uint8_t mac[6];
  WiFi.macAddress(mac);
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  doc["MessageID"] = messageID++;
  doc["FwVerEsp"] = FW_VERSION;
  // doc["InternalTemperature"] = String(readInternalTemperature(), 1) + "C";
  doc["ip"] = WiFi.localIP().toString();
  doc["Time"] = timeClient.getFormattedTime();
  doc["GPIOStatus"] = digitalRead(GPIO_PIN);
  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);
  String topic = "status/" + String(macStr);
  client.publish(topic.c_str(), jsonBuffer);
  if (uartEnabled) Serial.println(jsonBuffer);
}

void setup() {
  Serial.begin(115200);
  pinMode(GPIO_PIN, OUTPUT);
  loadConfig();
  if (wifiSSID == "") {
    setupWebServer();
    while (true) {
      server.handleClient();
      delay(10);
    }
  } else {
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) delay(500);
    if (WiFi.status() != WL_CONNECTED) ESP.restart();
    client.setServer(mqttServer.c_str(), mqttPort);
    client.setCallback(callback);
    timeClient.begin();
  }
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();
  timeClient.update();
  if (millis() - lastMsg > 5000) {
    lastMsg = millis();
    publishStatus();
  }
}
