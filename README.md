# ESP8266 Pomodoro Desk Clock (v2.0)

A professional, power-optimized productivity desk clock built with the ESP8266 (NodeMCU). This project balances precise NTP-synced timekeeping with a Pomodoro timer that **automatically logs your productivity sessions to Google Sheets** for long-term tracking.

## Core Philosophy: Aggressive Power Optimization
This project focuses heavily on maximizing efficiency on the ESP8266 without sacrificing responsiveness.

Instead of standard "Deep Sleep" (which kills the display) or "Light Sleep" (which can be unstable with WiFi), this project uses a custom **WiFi Duty Cycling Strategy**:
* **Radio Silence:** The WiFi radio is completely disabled (`WiFi.forceSleepBegin()`) during 99% of the operation (Clock & Timer modes). This drops power consumption drastically while keeping the OLED display and button interrupt active.
* **Just-in-Time Wake:** WiFi is only biologically "woken up" (`WiFi.forceSleepWake()`) for a few seconds *specifically* when a Focus session ends to log data to the cloud.
* **Zero-Latency Interface:** By keeping the CPU active (but radio off), the button remains instantly responsive, avoiding the "waking up" lag found in deep-sleep implementations.

## Features
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

## Google Sheets Logger Setup
This clock communicates with a Google Apps Script to log your data. Follow these steps to set it up:

### 1. Create the Google Sheet
1.  Go to [Google Sheets](https://sheets.google.com) and create a new sheet named `Pomodoro Logs`.
2.  Rename the first tab at the bottom to `Sheet1` (This is case-sensitive!).

### 2. Deploy the Script
1.  In your Google Sheet, go to **Extensions > Apps Script**.
2.  Delete any code there and paste the code from the block below.
3.  Click the **Floppy Disk icon** to Save.
4.  Click the blue **Deploy** button > **New Deployment**.
5.  **Select type:** Click the gear icon ⚙️ and select **Web app**.
6.  **Configuration:**
    * **Description:** `Pomodoro Logger`
    * **Execute as:** `Me (your_email@gmail.com)`
    * **Who has access:** `Anyone` (This is crucial so the ESP8266 can access it without OAuth).
7.  Click **Deploy**.
8.  **Copy the Web App URL** (It ends with `/exec`). You will need this for the C++ code.

#### Google Apps Script Code
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
## How to Run the Code
1.  **Clone this repository** and open it in **VS Code** with **PlatformIO**.
2.  **Configure Credentials:**
    * Create a file named `include/wifi_cred.h`.
    * Add your WiFi credentials (this file is ignored by Git for security):
    ```cpp
    #ifndef WIFI_CRED_H
    #define WIFI_CRED_H
    const char* WIFI_SSID = "YOUR_WIFI_NAME";
    const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
    #endif
    ```
3.  **Configure Logger:**
    * Open `src/GoogleLogger.cpp`.
    * Paste your **Web App URL** into the `GOOGLE_SCRIPT_URL` variable.
4.  **Upload** to your board!

## Libraries Used
* Adafruit SSD1306 & GFX
* NTPClient
* ESP8266HTTPClient (Core)
* WiFiClientSecure (Core)
