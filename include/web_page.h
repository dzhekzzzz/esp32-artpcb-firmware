#ifndef WEB_PAGE_H
#define WEB_PAGE_H

const char htmlPage[] = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Config</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 0;
      background-color: #f4f4f9;
      color: #333;
    }
    header {
      background-color: #4CAF50;
      color: white;
      padding: 10px 0;
      text-align: center;
    }
    h2 {
      font-size: 24px;
    }
    form {
      background-color: white;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
      max-width: 400px;
      margin: 30px auto;
    }
    label {
      font-size: 14px;
      margin-bottom: 8px;
      display: block;
    }
    input[type="text"],
    input[type="password"],
    input[type="number"] {
      width: 100%;
      padding: 10px;
      margin-bottom: 15px;
      border: 1px solid #ccc;
      border-radius: 4px;
      box-sizing: border-box;
    }
    input[type="submit"] {
      background-color: #4CAF50;
      color: white;
      border: none;
      padding: 10px 20px;
      font-size: 16px;
      border-radius: 4px;
      cursor: pointer;
      width: 100%;
    }
    input[type="submit"]:hover {
      background-color: #45a049;
    }
    .footer {
      text-align: center;
      font-size: 12px;
      color: #777;
      margin-top: 30px;
    }
  </style>
</head>
<body>
  <header>
    <h2>Налаштування ESP32</h2>
  </header>
  
  <form action="/save" method="GET">
    <label for="wifiSSID">Wi-Fi SSID:</label>
    <input type="text" id="wifiSSID" name="wifiSSID" required><br><br>

    <label for="wifiPassword">Wi-Fi Password:</label>
    <input type="password" id="wifiPassword" name="wifiPassword" required><br><br>

    <label for="mqttServer">MQTT Broker Address:</label>
    <input type="text" id="mqttServer" name="mqttServer" required><br><br>

    <label for="mqttPort">MQTT Port:</label>
    <input type="number" id="mqttPort" name="mqttPort" value="8883" required><br><br>

    <label for="mqttUser">MQTT Username:</label>
    <input type="text" id="mqttUser" name="mqttUser"><br><br>

    <label for="mqttPass">MQTT Password:</label>
    <input type="password" id="mqttPass" name="mqttPass"><br><br>

    <input type="submit" value="Зберегти">
  </form>

  <div class="footer">
    <p>&copy; 2025 ArtPCB test task</p>
  </div>
</body>
</html>
)rawliteral";

#endif
