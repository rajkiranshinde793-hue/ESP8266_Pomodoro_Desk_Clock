# ESP8266 Pomodoro Desk Clock (v2.0)

A professional, power-optimized productivity desk clock built with the ESP8266 (NodeMCU). This project balances precise NTP-synced timekeeping with a Pomodoro timer that **automatically logs your productivity sessions to Google Sheets** and **syncs "Do Not Disturb" mode to your Android phone**.

## Core Philosophy: Aggressive Power Optimization
This project focuses heavily on maximizing efficiency on the ESP8266 without sacrificing responsiveness.

Instead of standard "Deep Sleep" (which kills the display) or "Light Sleep" (which can be unstable with WiFi), this project uses a custom **WiFi Duty Cycling Strategy**:
* **Radio Silence:** The WiFi radio is completely disabled (`WiFi.forceSleepBegin()`) during 99% of the operation (Clock & Timer modes). This drops power consumption drastically while keeping the OLED display and button interrupt active.
* **Just-in-Time Wake:** WiFi is only biologically "woken up" (`WiFi.forceSleepWake()`) for a few seconds *specifically* when a Focus session ends to log data to the cloud or trigger DND.
* **Zero-Latency Interface:** By keeping the CPU active (but radio off), the button remains instantly responsive, avoiding the "waking up" lag found in deep-sleep implementations.

## Features
* **Phone DND Sync (New):** Automatically puts your Android phone into "Do Not Disturb" mode when a focus session starts and reverts it when the session ends (via MacroDroid).
* **Automated Cloud Logging:** Every completed 25-minute focus session is instantly logged to a private Google Sheet with a timestamp.
* **Smart Daily Stats:** The Google Sheet automatically formats data into daily clusters, calculating total hours focused per day.
* **WiFi & NTP Sync:** Automatically fetches precise time from `pool.ntp.org`.
* **OLED Display:** Shows time, date, day, and session count on a 128x64 SSD1306 screen.
* **Pomodoro Timer:**
    * **Focus Mode:** 25 Minutes (Short press).
    * **Break Mode:** 5 Minutes (Automatic after focus).
    * **Reset:** Long press to reset sessions.
* **Data Persistence:** Local backup of session counts to EEPROM prevents data loss on reboot.

## Hardware Required
* **NodeMCU ESP8266** (or similar ESP-12E board)
* **0.96" OLED Display** (I2C)
* **Push Button** (Connected to Pin D5)
* **Active Buzzer** (Connected to Pin D6)

## Wiring
| Component | ESP8266 Pin |
| :--- | :--- |
| **OLED SDA** | D2 (GPIO 4) |
| **OLED SCL** | D1 (GPIO 5) |
| **Button** | D5 (GPIO 14) |
| **Buzzer** | D6 (GPIO 12) |

---

## Configuration & Setup

### 1. The `credentials.h` File
For security, all sensitive data (WiFi, API keys) is stored in a separate file that is ignored by Git.

1.  Clone this repository.
2.  Create a new file named `credentials.h` in the same folder as your `.ino` (or `src`) file.
3.  Copy the code below and fill in your details:

```cpp
#pragma once
#ifndef WIFI_CRED_H
#define WIFI_CRED_H

// 1. WiFi Credentials
static const char* WIFI_SSID     = "YOUR_WIFI_NAME";
static const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// 2. MacroDroid Webhook URLs (For Phone DND Integration)
// See "MacroDroid Setup" section below to get these links
static const char* URL_DND_ON  = "[https://trigger.macrodroid.com/YOUR_ID/DND](https://trigger.macrodroid.com/YOUR_ID/DND)";
static const char* URL_DND_OFF = "[https://trigger.macrodroid.com/YOUR_ID/DNDOFF](https://trigger.macrodroid.com/YOUR_ID/DNDOFF)";

// 3. Google App Script URL (For Logging)
// See "Google Sheets Setup" section below
static const char* GOOGLE_SCRIPT_URL = "[https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec](https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec)"; 

#endif
```

---

### 2. Google Sheets Logger Setup
This clock communicates with a Google Apps Script to log your data. 

1.  Go to [Google Sheets](https://sheets.google.com) and create a new sheet named `Pomodoro Logs`.
2.  Rename the first tab at the bottom to `Sheet1` (Case-sensitive!).
3.  Go to **Extensions > Apps Script**.
4.  Paste the code found in the **[Google Apps Script Code](#google-apps-script-code)** section below.
5.  Click **Deploy** > **New Deployment**.
6.  **Configuration:**
    * **Type:** Web App
    * **Execute as:** Me
    * **Who has access:** **Anyone** (Crucial for ESP8266 access).
7.  Copy the **Web App URL** and paste it into `credentials.h` as `GOOGLE_SCRIPT_URL`.

---

### 3. MacroDroid Setup (Phone DND Integration)
To enable the clock to silence your phone, we use the MacroDroid automation app (Android).

1.  **Install App:** Download [MacroDroid](https://play.google.com/store/apps/details?id=com.arlosoft.macrodroid) from the Play Store.
2.  **Create "DND ON" Macro:**
    * **Trigger:** Select `Connectivity` > `Webhook (URL)`.
    * **Identifier:** Type `DND`.
    * **Action:** Select `Volume` > `Priority Mode/Do Not Disturb` > `Block All`.
    * **Save:** Name the macro "Pomodoro DND ON".
3.  **Create "DND OFF" Macro:**
    * **Trigger:** Select `Webhook (URL)`.
    * **Identifier:** Type `DNDOFF`.
    * **Action:** Select `Volume` > `Priority Mode/Do Not Disturb` > `Allow All`.
    * **Save:** Name the macro "Pomodoro DND OFF".
4.  **Get the URLs:**
    * Open your "DND ON" macro, click the Trigger (Webhook), and copy the URL shown. It will look like: `https://trigger.macrodroid.com/.../DND`.
    * Paste this into `credentials.h` as `URL_DND_ON`.
    * Do the same for `URL_DND_OFF`.

---

## Google Apps Script Code
Use this code for Step 2 of the setup.

```javascript
function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Sheet1");
  var data = JSON.parse(e.postData.contents);
   
  // 1. Get Current Date & Time
  var now = new Date();
  var todayDateStr = Utilities.formatDate(now, "GMT+5:30", "dd-MM-yyyy");
  var timeStr = Utilities.formatDate(now, "GMT+5:30", "HH:mm");
   
  // 2. Find the Last Header to organize daily stats
  var lastRow = sheet.getLastRow();
  var lastHeaderRow = 0;
  var lastHeaderDate = "";
   
  // Efficiently scan last 50 rows
  if (lastRow > 0) {
    var startScan = Math.max(1, lastRow - 50);
    var columnData = sheet.getRange(startScan, 1, lastRow - startScan + 1).getValues();
     
    for (var i = columnData.length - 1; i >= 0; i--) {
      var cellValue = String(columnData[i][0]);
      if (cellValue.indexOf("Stats of") === 0) {
        lastHeaderRow = startScan + i;
        lastHeaderDate = cellValue.replace("Stats of ", "").trim();
        break;
      }
    }
  }

  // --- LOGIC: NEW DAY vs SAME DAY ---
  if (lastHeaderDate !== todayDateStr) {
    // SCENARIO A: NEW DAY - Close out previous day stats
    if (lastHeaderRow > 0) {
      var firstSessionRow = lastHeaderRow + 2; 
      var sessionCount = 0;
      if (lastRow >= firstSessionRow) {
        sessionCount = lastRow - firstSessionRow + 1;
      }
       
      var totalMinutes = sessionCount * 25;
      var totalHours = (totalMinutes / 60).toFixed(1);
       
      sheet.appendRow(["--", "--"]);
      sheet.appendRow(["Total", totalMinutes + " mins"]);
      sheet.appendRow(["", "~ " + totalHours + " Hours"]);
      sheet.appendRow([" ", " "]);
    }
     
    // Create Header for Today
    sheet.appendRow(["Stats of " + todayDateStr]);
    sheet.getRange(sheet.getLastRow(), 1).setFontWeight("bold");
    sheet.appendRow(["Time", "Session"]);
    sheet.getRange(sheet.getLastRow(), 1, 1, 2).setFontWeight("bold");
    sheet.appendRow([timeStr, 1]); 
     
  } else {
    // SCENARIO B: SAME DAY - Increment session count
    var lastSessionNum = 1;
    try {
      var val = sheet.getRange(lastRow, 2).getValue();
      if (!isNaN(parseFloat(val))) {
        lastSessionNum = Number(val) + 1;
      }
    } catch(err) { lastSessionNum = 1; }
     
    sheet.appendRow([timeStr, lastSessionNum]);
    sheet.getRange(sheet.getLastRow(), 2).setNumberFormat("0");
  }
   
  return ContentService.createTextOutput("Success").setMimeType(ContentService.MimeType.TEXT);
}
```

## How to Run
1.  Open the project in **VS Code** with **PlatformIO**.
2.  Ensure your `credentials.h` file is created and populated.
3.  Connect your ESP8266 via USB.
4.  Upload the code!

## Libraries Used
* Adafruit SSD1306 & GFX
* NTPClient
* ESP8266HTTPClient (Core)
* WiFiClientSecure (Core)
