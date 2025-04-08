#ifndef WEB_PAGE_H
#define WEB_PAGE_H

const char htmlPage[] = R"rawliteral(
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

#endif