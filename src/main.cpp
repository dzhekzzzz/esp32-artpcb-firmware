#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>




#define GPIO_PIN 2
#define FW_VERSION "1.0.0"


const char* defaultSSID = "ESP32-AP";
const char* defaultPassword = "12345678";


WebServer server(80); 
WiFiClientSecure espClient;
espClient.setInsecure(); 


String wifiSSID = "";
String wifiPassword = "";
String mqttServer = "";
int mqttPort = 8883;
String mqttUser = "";  
String mqttPass = "";

Preferences preferences;

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><title>ESP32 Config</title></head>
    <body>
      <h2>ESP32 Configuration</h2>
      <form action="/save" method="GET">
        Wi-Fi SSID: <input type="text" name="wifiSSID"><br><br>
        Wi-Fi Password: <input type="password" name="wifiPassword"><br><br>
        MQTT Broker Address: <input type="text" name="mqttServer"><br><br>
        MQTT Port: <input type="number" name="mqttPort" value="8883"><br><br>
        MQTT Username: <input type="text" name="mqttUser"><br><br>
        MQTT Password: <input type="password" name="mqttPass"><br><br>
        <input type="submit" value="Save">
      </form>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleSave() {
  wifiSSID = server.arg("wifiSSID");
  wifiPassword = server.arg("wifiPassword");
  mqttServer = server.arg("mqttServer");
  mqttPort = server.arg("mqttPort").toInt();
  mqttUser = server.arg("mqttUser");
  mqttPass = server.arg("mqttPass");

  preferences.begin("config", false);
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPassword", wifiPassword);
  preferences.putString("mqttServer", mqttServer);
  preferences.putInt("mqttPort", mqttPort);
  preferences.putString("mqttUser", mqttUser);
  preferences.putString("mqttPass", mqttPass);
  preferences.end();

  server.send(200, "text/html", "<h2>Settings saved. Restarting...</h2>");

  delay(2000);
  ESP.restart();
}
void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected to Wi-Fi, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Message: " + message);
}


void connectToMQTT() {
  Serial.println("Connecting to MQTT...");
  mqttClient.setServer(mqttServer.c_str(), mqttPort);
  mqttClient.setCallback(mqttCallback);

  int attempts = 0;
  while (!mqttClient.connected() && attempts < 20) {
    Serial.print(".");
    if (mqttClient.connect("ESP32Client", mqttUser.c_str(), mqttPass.c_str())) {
      Serial.println("Connected to MQTT broker.");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      delay(5000);
    }
    attempts++;
  }
}

void setup() {
  Serial.begin(115200);
  preferences.begin("config", true);
  wifiSSID = preferences.getString("wifiSSID", "");
  wifiPassword = preferences.getString("wifiPassword", "");
  mqttServer = preferences.getString("mqttServer", "");
  mqttPort = preferences.getInt("mqttPort", 8883);
  mqttUser = preferences.getString("mqttUser", "");
  mqttPass = preferences.getString("mqttPass", "");
  preferences.end();

  if (wifiSSID == "") {
    Serial.println("First time setup, starting AP...");
    WiFi.softAP("ESP32-Setup", "12345678");
    Serial.print("Access Point IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.begin();
    Serial.println("HTTP server started.");
    return; 
  }

  connectToWiFi();
  connectToMQTT();
}

void loop() {
  server.handleClient();
  mqttClient.loop(); 
}
