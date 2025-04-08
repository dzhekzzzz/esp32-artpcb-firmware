#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Визначення пінів та версії прошивки
#define GPIO_PIN 2
#define FW_VERSION "1.0.0"

// Стандартні налаштування Wi-Fi та MQTT
const char* defaultSSID = "ESP32-AP";
const char* defaultPassword = "12345678";

// Оголошення об'єктів
WebServer server(80);
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

// Змінні для налаштувань
String wifiSSID = "";
String wifiPassword = "";
String mqttServer = "";
int mqttPort = 8883; // Порт для TLS-з'єднання
String mqttUser = "";
String mqttPass = "";

// Ініціалізація Preferences для збереження налаштувань у флеш-пам'яті
Preferences preferences;

// Ініціалізація NTP
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 3600, 60000); // CET = UTC+1, 3600 секунд


static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// Функція для обробки запиту на головну сторінку
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

// Функція для збереження налаштувань
void handleSave() {
  wifiSSID = server.arg("wifiSSID");
  wifiPassword = server.arg("wifiPassword");
  mqttServer = server.arg("mqttServer");
  mqttPort = server.arg("mqttPort").toInt();
  mqttUser = server.arg("mqttUser");
  mqttPass = server.arg("mqttPass");

  // Збереження налаштувань у флеш-пам'яті
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

// Підключення до Wi-Fi
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
    Serial.println("\nConnected to Wi-Fi, IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWi-Fi connection failed");
  }
}

// Callback-функція для обробки вхідних MQTT-повідомлень
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Message: " + message);
}

// Підключення до MQTT
void connectToMQTT() {
  Serial.println("Connecting to MQTT...");

  secureClient.setCACert(root_ca); // Встановлення кореневого сертифіката
  mqttClient.setServer(mqttServer.c_str(), mqttPort);
  mqttClient.setCallback(mqttCallback);

  int attempts = 0;
  while (!mqttClient.connected() && attempts < 20) {
    Serial.print("Attempt ");
    Serial.print(attempts + 1);
    Serial.print(": ");

    if (mqttClient.connect("ESP32Client", mqttUser.c_str(), mqttPass.c_str())) {
      Serial.println("Connected to MQTT broker.");
    } else {
      Serial.print("Failed, rc=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
    attempts++;
  }

  if (!mqttClient.connected()) {
    Serial.println("Unable to connect to MQTT broker after multiple attempts.");
  }
}

// Функція для публікації даних через MQTT
void publishData() {
  String macAddress = WiFi.macAddress();
  macAddress.replace(":", ""); // Виправлення формату MAC-адреси

  // Отримання часу за допомогою NTP
  timeClient.update();
  String currentTime = timeClient.getFormattedTime();

  // Отримання IP-адреси
  String ip = WiFi.localIP().toString();

  // Отримання внутрішньої температури
  float internalTemperature = 25.0; // Потрібно підключити датчик температури для реального вимірювання

  // Створення JSON-об'єкта
  DynamicJsonDocument doc(1024);
  doc["MessageID"] = 1001;
  doc["FwVerEsp"] = FW_VERSION;
  doc["InternalTemperature"] = String(internalTemperature) + "C";
  doc["ip"] = ip;
  doc["Time"] = currentTime;
  doc["GPIOStatus"] = digitalRead(GPIO_PIN) == HIGH ? "1" : "0";

  // Публікація даних через MQTT
  String jsonString;
  serializeJson(doc, jsonString);
  String topic = "status/" + macAddress;
  mqttClient.publish(topic.c_str(), jsonString.c_str());

  Serial.println("Data published: " + jsonString);
}

// Функція setup
void setup() {
  Serial.begin(115200);

  // Завантаження збережених налаштувань
  preferences.begin("config", true);
  wifiSSID = preferences.getString("wifiSSID", "");
  wifiPassword = preferences.getString("wifiPassword", "");
  mqttServer = preferences.getString("mqttServer", "");
  mqttPort = preferences.getInt("mqttPort", 8883);
  mqttUser = preferences.getString("mqttUser", "");
  mqttPass = preferences.getString("mqttPass", "");
  preferences.end();

  if (wifiSSID.isEmpty()) {
    // Якщо налаштування не збережені, запускаємо точку доступу для первинного налаштування
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

  // Підключення до Wi-Fi та MQTT після первинного налаштування
  connectToWiFi();
  connectToMQTT();

  // Ініціалізація NTP
  timeClient.begin();
}

// Функція loop
void loop() {
  server.handleClient(); // Обробка HTTP-запитів
  mqttClient.loop(); // Обробка MQTT-повідомлень

  // Публікація даних кожні 5 секунд
  static unsigned long lastPublishTime = 0;
  if (millis() - lastPublishTime >= 5000) {
    publishData();
    lastPublishTime = millis();
  }
}