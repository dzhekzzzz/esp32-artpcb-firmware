# ESP32 Firmware for ArtPCB Test Task

Firmware for ESP32-WROOM-32 with MQTT and GPIO functionality.  
Built using PlatformIO.

## 🚧 Project Status

This project is currently in **alpha**. It's functional, but may require further refinement and testing.

## 🧩 About

This was a moderately complex task for me, especially since I had no prior experience with MQTT. It took some time to understand the protocol and implement it correctly, but the process was very interesting and rewarding.

As the MQTT broker, I used **HiveMQ’s public broker** for testing and development.  
I faced some issues connecting to the server at first, but managed to resolve them after troubleshooting.

## ⚙️ Features

- Secure MQTT connection over TLS (port 8883)
- Web interface for configuring Wi-Fi and MQTT settings
- Persistent storage of preferences using `Preferences.h`
- GPIO and UART control via MQTT messages
- Time synchronization using NTP

## 🧾 Notes on the Code

The firmware initializes a secure MQTT client, connects to Wi-Fi, synchronizes time via NTP, and sets up a simple web server for managing configuration. All key settings are stored in non-volatile memory to persist between reboots. Commands received via MQTT allow remote control over GPIO and UART functionality.

## 💬 Feedback

I'm open to feedback and would appreciate any suggestions or code reviews.  
If you notice potential bugs, inefficiencies, or areas for improvement, I’d be glad to hear your perspective.


## 📦 How to Build

This project uses PlatformIO as the build system. To build and upload the firmware, follow these steps:

1. Install [PlatformIO](https://platformio.org/).
2. Open the project folder in your preferred IDE (e.g. VSCode with PlatformIO extension).
3. Build the project using the PlatformIO build command.
4. Connect your ESP32-WROOM-32 device.
5. Upload the firmware to the device.

---

This README provides a high-level overview of the project, details some of the key functionalities, and explains the challenges I faced and lessons learned during development.
