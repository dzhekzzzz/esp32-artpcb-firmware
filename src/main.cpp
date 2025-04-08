#include "root_ca.h"
#include "web_page.h"
#include "config.h"

#define GPIO_PIN 2
#define FW_VERSION "1.0.0"

void handleRoot() {
  server.send(200, "text/html", htmlPage); // Serve the configuration page
}

// Save settings to flash memory
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
  ESP.restart(); // Restart after saving settings
}

// MQTT callback to handle messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == "gpio/control") {
    digitalWrite(gpioPin, message == "1" ? HIGH : LOW);
    Serial.println(message == "1" ? "GPIO Pin ON" : "GPIO Pin OFF");
  }

  if (String(topic) == "uart/control") {
    uartEnabled = message == "1";
    Serial.println(uartEnabled ? "UART Output Enabled" : "UART Output Disabled");
  }
}

// Connect to Wi-Fi
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

// Connect to MQTT broker
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

// Publish data to MQTT
void publishData() {
  String macAddress = WiFi.macAddress();
  macAddress.replace(":", "");

  timeClient.update();
  String currentTime = timeClient.getFormattedTime();

  String ip = WiFi.localIP().toString();
  float internalTemperature = temperatureRead(); 

  DynamicJsonDocument doc(1024);
  doc["MessageID"] = 1001;
  doc["FwVerEsp"] = FW_VERSION;
  doc["InternalTemperature"] = String(internalTemperature) + "C";
  doc["ip"] = ip;
  doc["Time"] = currentTime;
  doc["GPIOStatus"] = digitalRead(GPIO_PIN) == HIGH ? "1" : "0";

  String jsonString;
  serializeJson(doc, jsonString);
  String topic = "status/" + macAddress;
  mqttClient.publish(topic.c_str(), jsonString.c_str());

  if (uartEnabled) {
    Serial.println("Data over UART: " + jsonString); // UART output
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(gpioPin, OUTPUT);
  digitalWrite(gpioPin, LOW);  // Initially turn off GPIO pin
  
  preferences.begin("config", true);
  wifiSSID = preferences.getString("wifiSSID", "");
  wifiPassword = preferences.getString("wifiPassword", "");
  mqttServer = preferences.getString("mqttServer", "");
  mqttPort = preferences.getInt("mqttPort", 8883);
  mqttUser = preferences.getString("mqttUser", "");
  mqttPass = preferences.getString("mqttPass", "");
  preferences.end();

  if (wifiSSID.isEmpty()) {
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
  timeClient.begin(); // Start NTP client
}

void loop() {
  server.handleClient();
  mqttClient.loop();

  static unsigned long lastPublishTime = 0;
  if (millis() - lastPublishTime >= 5000) {
    publishData();
    lastPublishTime = millis();
  }
}
