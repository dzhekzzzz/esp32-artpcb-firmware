#include "root_ca.h"
#include "web_page.h"
#include "config.h"

// Константи і налаштування
#define GPIO_PIN 2
#define FW_VERSION "1.0.0"

// Прототипи функцій HTTP-обробників
void handleRoot();
void handleSave();

// Прототипи функцій MQTT та Wi-Fi
void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectToWiFi();
void connectToMQTT();
void publishData();

// Обробник головної сторінки (HTTP)
void handleRoot() {
  // Відправка html-сторінки для налаштувань
  server.send(200, "text/html", htmlPage);
}

// Обробник збереження налаштувань
void handleSave() {
  // Зчитування значень параметрів з HTTP запиту
  wifiSSID     = server.arg("wifiSSID");
  wifiPassword = server.arg("wifiPassword");
  mqttServer   = server.arg("mqttServer");
  mqttPort     = server.arg("mqttPort").toInt();
  mqttUser     = server.arg("mqttUser");
  mqttPass     = server.arg("mqttPass");

  // Збереження налаштувань у флеш-пам'ять
  preferences.begin("config", false);
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPassword", wifiPassword);
  preferences.putString("mqttServer", mqttServer);
  preferences.putInt("mqttPort", mqttPort);
  preferences.putString("mqttUser", mqttUser);
  preferences.putString("mqttPass", mqttPass);
  preferences.end();

  // Відправка відповіді клієнту та перезапуск пристрою
  server.send(200, "text/html", "<h2>Settings saved. Restarting...</h2>");
  delay(2000);
  ESP.restart();
}

// MQTT callback для обробки повідомлень
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  // Збирання вхідного повідомлення
  for (unsigned int i = 0; i < length; i++) {
    message += static_cast<char>(payload[i]);
  }

  // Управління GPIO
  if (String(topic) == "gpio/control") {
    digitalWrite(GPIO_PIN, message == "1" ? HIGH : LOW);
    Serial.println(message == "1" ? "GPIO Pin ON" : "GPIO Pin OFF");
  }
  // Управління UART виводом
  else if (String(topic) == "uart/control") {
    uartEnabled = (message == "1");
    Serial.println(uartEnabled ? "UART Output Enabled" : "UART Output Disabled");
  }
}

// Функція підключення до Wi-Fi
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

// Функція підключення до MQTT-брокера
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

// Публікація даних до MQTT
void publishData() {
  // Генерація унікального MAC-адреси без символів ':'
  String macAddress = WiFi.macAddress();
  macAddress.replace(":", "");

  // Отримання поточного часу через NTP-клієнт
  timeClient.update();
  String currentTime = timeClient.getFormattedTime();

  String ip = WiFi.localIP().toString();
  float internalTemperature = temperatureRead(); 

  // Формування JSON повідомлення
  DynamicJsonDocument doc(1024);
  doc["MessageID"] = 1001;
  doc["FwVerEsp"] = FW_VERSION;
  doc["InternalTemperature"] = String(internalTemperature) + "C";
  doc["ip"] = ip;
  doc["Time"] = currentTime;
  doc["GPIOStatus"] = (digitalRead(GPIO_PIN) == HIGH ? "1" : "0");

  String jsonString;
  serializeJson(doc, jsonString);
  
  // Публікація повідомлення на MQTT-топік
  String topic = "status/" + macAddress;
  mqttClient.publish(topic.c_str(), jsonString.c_str());

  // Якщо UART увімкнено - вивід даних
  if (uartEnabled) {
    Serial.println("Data over UART: " + jsonString);
  }
}

// Налаштування мікроконтролера
void setup() {
  Serial.begin(115200);

  // Вивід частотних характеристик пристрою
  Serial.print("XTAL frequency: ");
  Serial.println(40 * 1000000); // Приблизно 40 МГц
  Serial.print("CPU frequency: ");
  Serial.println(ESP.getCpuFreqMHz() * 1000000); // Частота ЦП (в Гц)
  Serial.print("APB frequency: ");
  Serial.println(80 * 1000000); // Приблизно 80 МГц

  // Налаштування GPIO
  pinMode(GPIO_PIN, OUTPUT);
  digitalWrite(GPIO_PIN, LOW);  // Початкове значення - вимкнено

  // Завантаження збережених налаштувань з пам'яті
  preferences.begin("config", true);
  wifiSSID     = preferences.getString("wifiSSID", "");
  wifiPassword = preferences.getString("wifiPassword", "");
  mqttServer   = preferences.getString("mqttServer", "");
  mqttPort     = preferences.getInt("mqttPort", 8883);
  mqttUser     = preferences.getString("mqttUser", "");
  mqttPass     = preferences.getString("mqttPass", "");
  preferences.end();

  // Якщо Wi-Fi налаштування не задані, запускаємо режим AP для первинного налаштування
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

  // Ініціалізація підключень
  connectToWiFi();
  connectToMQTT();
  timeClient.begin(); // Запуск NTP-клієнта
}

// Основний цикл програми
void loop() {
  server.handleClient();
  mqttClient.loop();

  static unsigned long lastPublishTime = 0;
  // Публікація даних кожні 5 секунд
  if (millis() - lastPublishTime >= 5000) {
    publishData();
    lastPublishTime = millis();
  }
}
