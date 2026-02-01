# ESP8266 Pomodoro Desk Clock

A professional productivity desk clock built with the ESP8266 (NodeMCU) and PlatformIO. It features a precise NTP-synced clock and a built-in Pomodoro timer to manage focus sessions.

## Features
* **WiFi & NTP Sync:** Automatically fetches precise time from `pool.ntp.org`.
* **OLED Display:** Shows time, date, day, and session count on a 128x64 SSD1306 screen.
* **Pomodoro Timer:**
    * **Focus Mode:** 25 Minutes (Short press).
    * **Break Mode:** 5 Minutes (Automatic after focus).
    * **Reset:** Long press to reset sessions.
* **Smart Power Management:** Uses Light Sleep to reduce power consumption.
* **Data Persistence:** Saves your daily session count to EEPROM so you don't lose progress after a reboot.

## 🛠️ Hardware Required
* **NodeMCU ESP8266** (or similar ESP-12E board)
* **0.96" OLED Display** (I2C)
* **Push Button** (Connected to Pin D5)
* **Active Buzzer** (Connected to Pin D6)

## 🔌 Wiring
| Component | ESP8266 Pin |
| :--- | :--- |
| **OLED SDA** | D2 (GPIO 4) |
| **OLED SCL** | D1 (GPIO 5) |
| **Button** | D5 (GPIO 14) |
| **Buzzer** | D6 (GPIO 12) |

## 🚀 How to Run
1.  Clone this repository.
2.  Open the folder in **VS Code** with **PlatformIO**.
3.  Create a file named `include/wifi_cred.h` (this file is ignored by Git for security):
    ```cpp
    #ifndef WIFI_CRED_H
    #define WIFI_CRED_H
    const char* WIFI_SSID = "YOUR_WIFI_NAME";
    const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
    #endif
    ```
4.  Upload to your board!

## 📦 Libraries Used
* Adafruit SSD1306 & GFX
* NTPClient
