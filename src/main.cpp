#include "root_ca.h"
#include "web_page.h"
#include "config.h"

#define GPIO_PIN 2
#define FW_VERSION "1.0.0"

// =====================================================
// FUNCTION PROTOTYPES
// =====================================================
void handleRoot();
void handleSave();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectToWiFi();
void connectToMQTT();
void publishData();
void startAccessPoint();
String getMacHex();

// =====================================================
// FUNCTION: handleRoot()
// PURPOSE: Serves the HTML root page
// NOTE: All data in web_page.h
// =====================================================
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// =====================================================
// FUNCTION: handleSave()
// PURPOSE: Saves Wi-Fi and MQTT settings to preferences
// =====================================================
void handleSave() {
  // Get settings from HTTP form
  wifiSSID     = server.arg("wifiSSID");
  wifiPassword = server.arg("wifiPassword");
  mqttServer   = server.arg("mqttServer");
  mqttPort     = server.arg("mqttPort").toInt();
  mqttUser     = server.arg("mqttUser");
  mqttPass     = server.arg("mqttPass");

  Serial.println("===== SETTINGS RECEIVED FROM FORM =====");
  Serial.println("Wi-Fi SSID: " + wifiSSID);
  Serial.println("Wi-Fi Password: " + wifiPassword);
  Serial.println("MQTT Server: " + mqttServer);
  Serial.println("MQTT Port: " + String(mqttPort));
  Serial.println("MQTT User: " + mqttUser);
  Serial.println("MQTT Password: " + mqttPass);
  Serial.println("========================================");

  // Store settings in preferences
  preferences.begin("config", false);
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPassword", wifiPassword);
  preferences.putString("mqttServer", mqttServer);
  preferences.putInt("mqttPort", mqttPort);
  preferences.putString("mqttUser", mqttUser);
  preferences.putString("mqttPass", mqttPass);
  preferences.end();

  // Inform the user and restart
  server.send(200, "text/html", "<h2>Settings saved. Restarting...</h2>");
  delay(2000);
  ESP.restart();
}

// =====================================================
// FUNCTION: mqttCallback()
// PURPOSE: Handles incoming MQTT messages
// =====================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }

  // Handle GPIO control
  if (String(topic) == "gpio/control") {
    digitalWrite(GPIO_PIN, message == "1" ? HIGH : LOW);
    Serial.println(message == "1" ? "GPIO Pin ON" : "GPIO Pin OFF");
  }
  // Handle UART control
  else if (String(topic) == "uart/control") {
    uartEnabled = (message == "1");
    Serial.println(uartEnabled ? "UART Output Enabled" : "UART Output Disabled");
  }
}

// =====================================================
// FUNCTION: connectToWiFi()
// PURPOSE: Connects to the Wi-Fi network
// =====================================================
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

// =====================================================
// FUNCTION: startAccessPoint()
// PURPOSE: Starts the Access Point mode for setup
// =====================================================
void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Setup", "12345678");
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
  Serial.println("HTTP server started.");
}

// =====================================================
// FUNCTION: connectToMQTT()
// PURPOSE: Connects to the MQTT broker
// =====================================================
void connectToMQTT() {
  Serial.println("Connecting to MQTT...");

  // Вибір клієнта в залежності від порту
  if (mqttPort == 8883) {
    Serial.println("Using secure MQTT (TLS)");
    secureClient.setCACert(root_ca);
    mqttClient.setClient(secureClient);
  } else {
    Serial.println("Using plain MQTT (no TLS)");
    mqttClient.setClient(plainClient);
  }

  mqttClient.setServer(mqttServer.c_str(), mqttPort);
  mqttClient.setCallback(mqttCallback);

  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5) {
    if (mqttClient.connect("ESP32Client", mqttUser.c_str(), mqttPass.c_str())) {
      Serial.println("Connected to MQTT broker.");
      mqttClient.subscribe("uart/control");
      mqttClient.subscribe("gpio/control");
    } else {
      Serial.print("MQTT connect attempt ");
      Serial.print(attempts + 1);
      Serial.println(" failed. Retrying...");
      delay(3000);
      attempts++;
    }
  }

  if (!mqttClient.connected()) {
    Serial.println("Unable to connect to MQTT broker. Switching to AP mode...");
    currentState = STATE_SETUP_AP;
  }
}

// =====================================================
// FUNCTION: getMacHex()
// PURPOSE: Retrieves the MAC address of the device in HEX format
// =====================================================
String getMacHex() {
  uint8_t mac[6];
  esp_base_mac_addr_get(mac);

  char macStr[13];
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(macStr);
}

// =====================================================
// FUNCTION: publishData()
// PURPOSE: Publishes device data to MQTT broker
// =====================================================
void publishData() {
  String macAddress = getMacHex();
  String ip = "Not connected";
  String currentTime = "Unavailable";
  float internalTemperature = temperatureRead();

  if (WiFi.status() == WL_CONNECTED) {
    ip = WiFi.localIP().toString();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char timeStr[10];
      strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
      currentTime = String(timeStr);
    } else {
      currentTime = "Time not synced";
    }
  }

  DynamicJsonDocument doc(1024);
  doc["MessageID"] = messageID++; //BUG FIX
  doc["FwVerEsp"] = FW_VERSION;
  doc["InternalTemperature"] = String(internalTemperature) + "C";
  doc["ip"] = ip;
  doc["Time"] = currentTime;
  doc["GPIOStatus"] = (digitalRead(GPIO_PIN) == HIGH ? "1" : "0");

  String jsonString;
  serializeJson(doc, jsonString);

  String topic = "status/" + macAddress;

  if (mqttClient.connected()) {
    mqttClient.publish(topic.c_str(), jsonString.c_str());
  }

  if (uartEnabled) {
    Serial.println("Data over UART: " + jsonString);
  }
}

// =====================================================
// FUNCTION: setup()
// PURPOSE: Initializes the system and connects to Wi-Fi/MQTT
// =====================================================
void setup() {
  Serial.begin(115200);

  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.nist.gov"); //TIME FIX

  pinMode(GPIO_PIN, OUTPUT);
  digitalWrite(GPIO_PIN, LOW);

  // Load configuration settings from preferences
  preferences.begin("config", true);
  wifiSSID     = preferences.getString("wifiSSID", "");
  wifiPassword = preferences.getString("wifiPassword", "");
  mqttServer   = preferences.getString("mqttServer", "");
  mqttPort     = preferences.getInt("mqttPort", 8883);
  mqttUser     = preferences.getString("mqttUser", "");
  mqttPass     = preferences.getString("mqttPass", "");
  preferences.end();


  // Determine connection state
  currentState = wifiSSID.isEmpty() ? STATE_SETUP_AP : STATE_CONNECT_WIFI;
}

// =====================================================
// FUNCTION: loop()
// PURPOSE: Main program loop that handles Wi-Fi, MQTT, and data publishing
// NOTE: I thought it would be better to rewrite this part of the code using a switch-case to make it more flexible and easier to understand.
// =====================================================
void loop() {
  server.handleClient();
  mqttClient.loop();

  switch (currentState) {
    case STATE_SETUP_AP:
      startAccessPoint();
      while (true) {
        server.handleClient();
        delay(10);
      }
      break;

    case STATE_CONNECT_WIFI:
      connectToWiFi();
      currentState = (WiFi.status() == WL_CONNECTED) ? STATE_CONNECT_MQTT : STATE_SETUP_AP;
      break;

    case STATE_CONNECT_MQTT:
      connectToMQTT();
      if (mqttClient.connected()) {
        configTzTime("Europe/Berlin", "pool.ntp.org", "time.nist.gov");
        currentState = STATE_RUNNING;
      }
      break;

    case STATE_RUNNING:
      static unsigned long lastPublishTime = 0;
      static unsigned long wifiCheckLast = 0;

      // Check Wi-Fi status
      if (millis() - wifiCheckLast > 10000) {
        wifiCheckLast = millis();
        if (WiFi.status() != WL_CONNECTED) {
          Serial.println("Wi-Fi connection lost. Restarting to enter AP mode...");
          delay(1000);
          ESP.restart();
        }
      }

      // Publish data every 5 seconds
      if (millis() - lastPublishTime >= 5000) {
        publishData();
        lastPublishTime = millis();
      }
      break;
  }
}
