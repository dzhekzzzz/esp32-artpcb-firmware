#include "root_ca.h"
#include "web_page.h"
#include "config.h"

// Some constants
#define GPIO_PIN 2
#define FW_VERSION "1.0.0"

// Func prototypes
void handleRoot();
void handleSave();

// Mqtt and Wifi func
void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectToWiFi();
void connectToMQTT();
void publishData();

//html setting page
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

//buffer
void handleSave() {
  wifiSSID     = server.arg("wifiSSID");
  wifiPassword = server.arg("wifiPassword");
  mqttServer   = server.arg("mqttServer");
  mqttPort     = server.arg("mqttPort").toInt();
  mqttUser     = server.arg("mqttUser");
  mqttPass     = server.arg("mqttPass");

  //Saving data to flash
  preferences.begin("config", false);
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPassword", wifiPassword);
  preferences.putString("mqttServer", mqttServer);
  preferences.putInt("mqttPort", mqttPort);
  preferences.putString("mqttUser", mqttUser);
  preferences.putString("mqttPass", mqttPass);
  preferences.end();

  //Ansver and board reload
  server.send(200, "text/html", "<h2>Settings saved. Restarting...</h2>");
  delay(2000);
  ESP.restart();
}

// MQTT callback 
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }

  // GPIO
  if (String(topic) == "gpio/control") {
    digitalWrite(GPIO_PIN, message == "1" ? HIGH : LOW);
    Serial.println(message == "1" ? "GPIO Pin ON" : "GPIO Pin OFF");
  }

  // UART 
  else if (String(topic) == "uart/control") {
    uartEnabled = (message == "1");
    Serial.println(uartEnabled ? "UART Output Enabled" : "UART Output Disabled");
  }
}

void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi, IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("Wi-Fi connection failed");
  }
}

void connectToMQTT() {
  Serial.println("Connecting to MQTT...");

  secureClient.setCACert(root_ca);
  mqttClient.setServer(mqttServer.c_str(), mqttPort);
  mqttClient.setCallback(mqttCallback);

  int attempts = 0;
  while (!mqttClient.connected() && attempts < 20) {
    if (mqttClient.connect("ESP32Client", mqttUser.c_str(), mqttPass.c_str())) {
      Serial.println("Connected to MQTT broker.");
      mqttClient.subscribe("uart/control");
      mqttClient.subscribe("gpio/control");
    } else {
      delay(5000);
      attempts++;
    }
  }

  if (!mqttClient.connected()) {
    Serial.println("Unable to connect to MQTT broker after multiple attempts.");
  }
}

String getMacHex() {
  uint8_t mac[6];
  WiFi.macAddress(mac);

  char macStr[13];
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X",
          mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);

  return String(macStr);
}


// Публікація даних до MQTT
void publishData() {
  String macAddress = getMacHex();

  // Getting TIME
  timeClient.update();
  String currentTime = timeClient.getFormattedTime();

  String ip = WiFi.localIP().toString();
  float internalTemperature = temperatureRead(); 

  // TO JSON 
  DynamicJsonDocument doc(1024);
  doc["MessageID"] = 1001;
  doc["FwVerEsp"] = FW_VERSION;
  doc["InternalTemperature"] = String(internalTemperature) + "C";
  doc["ip"] = ip;
  doc["Time"] = currentTime;
  doc["GPIOStatus"] = (digitalRead(GPIO_PIN) == HIGH ? "1" : "0");

  String jsonString;
  serializeJson(doc, jsonString);
  
  String topic = "status/" + macAddress;
  mqttClient.publish(topic.c_str(), jsonString.c_str());

  // Output data if UART is enabled
  if (uartEnabled) {
    Serial.println("Data over UART: " + jsonString);
  }
}


void setup() {                   
  Serial.begin(115200);

 
  Serial.print("XTAL frequency: ");
  Serial.println(40 * 1000000); // TBH IDK but usualy its around 40 МГц
  Serial.print("CPU frequency: ");
  Serial.println(ESP.getCpuFreqMHz() * 1000000); 
  Serial.print("APB frequency: ");
  Serial.println(80 * 1000000); // Same as XTAL

  // GPIO setup
  pinMode(GPIO_PIN, OUTPUT);
  digitalWrite(GPIO_PIN, LOW); 

  preferences.begin("config", true);
  wifiSSID     = preferences.getString("wifiSSID", "");
  wifiPassword = preferences.getString("wifiPassword", "");
  mqttServer   = preferences.getString("mqttServer", "");
  mqttPort     = preferences.getInt("mqttPort", 8883);                // TODO: remove ugly code frome here to somewhere else 
  mqttUser     = preferences.getString("mqttUser", "");
  mqttPass     = preferences.getString("mqttPass", "");
  preferences.end();

  if (wifiSSID.isEmpty()) {
    WiFi.softAP("ESP32-Setup", "12345678");
    Serial.print("Access Point IP: ");
    Serial.println(WiFi.softAPIP());                                                                 
    server.on("/", handleRoot);                                       // TODO: remove ugly code frome here to somewhere else 
    server.on("/save", handleSave);
    server.begin();
    Serial.println("HTTP server started.");
    return;
  }

  // Init
  connectToWiFi();
  connectToMQTT();
  timeClient.begin(); 
}


void loop() {
  server.handleClient();
  mqttClient.loop();

  static unsigned long lastPublishTime = 0;
  // Publish every 5 sec
  if (millis() - lastPublishTime >= 5000) {
    publishData();
    lastPublishTime = millis();
  }
}
