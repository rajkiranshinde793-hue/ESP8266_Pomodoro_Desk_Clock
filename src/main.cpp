#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Wire.h> 


#include "Config.h"
#include "wifi_cred.h"  

// Required for Sleep Functions & CPU Freq 
extern "C" {
  #include "user_interface.h"
}

// Custom Fonts 
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

// --- OBJECT INITIALIZATION ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", NTP_OFFSET);

// --- VARIABLES ---
String weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}; 

// State Variables 
Mode currentMode = CLOCK;
NetworkState netState = NET_IDLE;

unsigned long timerStartTime = 0;
unsigned long timerDuration = 0; 
bool lastButtonState = HIGH;
unsigned long buttonPressedTime = 0;
int sessionCount = 0;
unsigned long lastSyncTime = 0;
unsigned long wifiConnectStartTime = 0;

// --- FUNCTION PROTOTYPES (REQUIRED FOR PLATFORMIO) ---
void setBrightness();
void killWiFi();
void wakeWiFi();
void triggerBuzzer(int beeps, int duration = 200);
void saveSession();
void clearSessions();
void handleNetwork();
void handleButton();
void smartSleep(unsigned long ms);
void drawClock();
void drawPomodoro();

// --- IMPLEMENTATION ---

void setBrightness() {
  uint8_t contrast = 100;
  uint8_t precharge = 0xF1;  
  Wire.beginTransmission(0x3C);
  Wire.write(0x00);     
  Wire.write(0x81);     
  Wire.write(contrast); 
  Wire.write(0xD9);     
  Wire.write(precharge);
  Wire.endTransmission();
}

void killWiFi() {
  WiFi.disconnect(true);  
  WiFi.mode(WIFI_OFF);    
  WiFi.forceSleepBegin(); 
  delay(1);
}

void wakeWiFi() {
  WiFi.forceSleepWake();  
  delay(1);               
  WiFi.mode(WIFI_STA);    
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Using constants from Secrets.h
}

void setup() {
  system_update_cpu_freq(80);
  Serial.begin(9600);
  
  wifi_set_sleep_type(LIGHT_SLEEP_T); 

  EEPROM.begin(4); 
  sessionCount = EEPROM.read(EEPROM_ADDR);
  if (sessionCount > MAX_SESSIONS) sessionCount = 0;

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    for(;;);
  }
  
  display.clearDisplay();
  setBrightness();
  
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(12, 30);
  display.print("STARTING CLOCK...");
  display.display();

  // --- Initial Blocking Sync ---
  wakeWiFi();
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.clearDisplay();
    display.setCursor(1, 28);
    display.print("BABY JUST A MOVEMENT"); 
    display.display();

    timeClient.begin();
    
    int ntpRetry = 0;
    bool syncSuccess = false;
    
    while (ntpRetry < 10 && !syncSuccess) {
      if (timeClient.forceUpdate()) {
         if (timeClient.getEpochTime() > 946684800) {
            syncSuccess = true;
         }
      }
      if (!syncSuccess) delay(500);
      ntpRetry++;
    }

    if (syncSuccess) {
       lastSyncTime = millis();
    } else {
       lastSyncTime = millis() - SYNC_INTERVAL + 10000;
       display.clearDisplay();
       display.setCursor(10, 30);
       display.print("NTP Failed!");
       display.display();
       delay(1000);
    }
    killWiFi();
  } else {
    display.clearDisplay();
    display.setCursor(10,30);
    display.print("WiFi Failed!");
    display.display();
    delay(1000);
    lastSyncTime = millis() - SYNC_INTERVAL + 10000;
    killWiFi();
  }

  currentMode = CLOCK;
  
  while(digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
}

void triggerBuzzer(int beeps, int duration) {
  for(int i = 0; i < beeps; i++) {
    tone(BUZZER_PIN, 1000);
    delay(duration);
    noTone(BUZZER_PIN);
    delay(100);
  }
}

void saveSession() {
  sessionCount++;
  if (sessionCount > MAX_SESSIONS) {
    sessionCount = 0;
  }
  EEPROM.write(EEPROM_ADDR, sessionCount);
  EEPROM.commit();
}

void clearSessions() {
  sessionCount = 0;
  EEPROM.write(EEPROM_ADDR, 0);
  EEPROM.commit();
  triggerBuzzer(1, 1000);
}

void handleNetwork() {
  unsigned long currentMillis = millis();
  switch (netState) {
    case NET_IDLE:
      if (currentMillis - lastSyncTime >= SYNC_INTERVAL) {
        Serial.println("Background Sync Start");
        wakeWiFi(); 
        wifiConnectStartTime = currentMillis;
        netState = NET_CONNECTING;
      }
      break;
    case NET_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        netState = NET_UPDATING;
      } 
      else if (currentMillis - wifiConnectStartTime > 15000) {
        killWiFi();
        lastSyncTime = currentMillis; 
        netState = NET_IDLE;
      }
      break;
    case NET_UPDATING:
      if (timeClient.forceUpdate()) {
         Serial.println("NTP Success");
      }
      lastSyncTime = millis();
      killWiFi(); 
      netState = NET_IDLE;
      break;
  }
}

void handleButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  unsigned long now = millis();
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    buttonPressedTime = now;
  }
  
  if (currentButtonState == HIGH && lastButtonState == LOW) {
    unsigned long pressDuration = now - buttonPressedTime;
    if (pressDuration >= LONG_PRESS_MS) {
      if (currentMode == CLOCK) {
        clearSessions();
      } else {
        currentMode = CLOCK;
        timerStartTime = 0;
        triggerBuzzer(2, 100);
      }
    } else if (pressDuration > 50) {
      if (currentMode == CLOCK) {
        currentMode = FOCUS;
        timerDuration = 25 * 60; 
        timerStartTime = now;
        triggerBuzzer(1); 
      }
    }
  }
  lastButtonState = currentButtonState;
}

void smartSleep(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    handleButton();
    handleNetwork(); 
    
    if (netState != NET_IDLE) {
       delay(10);
    } else {
       delay(50);
    }

    if (currentMode != CLOCK) return;
  }
}

void loop() {
  unsigned long loopStart = millis();

  display.clearDisplay();

  if (currentMode == CLOCK) {
    drawClock();
    handleNetwork();
    unsigned long executionTime = millis() - loopStart;
    if (executionTime < 1000) {
      smartSleep(1000 - executionTime);
    }
  } else {
    handleButton();
    drawPomodoro();
    delay(10); 
  }

  display.display();
}

void drawClock() {
  display.setFont();
  display.setTextSize(1);
  display.setCursor(75, 0);
  display.print("Sess: ");
  display.print(sessionCount);

  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(5, 30); 
  int rawHours = timeClient.getHours();
  int displayHours = rawHours % 12;
  if (displayHours == 0) displayHours = 12;
  
  char timeBuffer[10];
  sprintf(timeBuffer, "%02d:%02d:%02d", displayHours, timeClient.getMinutes(), timeClient.getSeconds());
  display.print(timeBuffer);

  display.setFont();
  display.setCursor(85, 20);
  display.print(rawHours >= 12 ? "PM" : "AM");

  display.drawFastHLine(0, 40, 128, SSD1306_WHITE);

  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti = localtime(&rawtime);
  
  display.setFont(&FreeMono9pt7b);
  display.setCursor(5, 56);
  display.print(weekDays[timeClient.getDay()]);

  display.setCursor(70, 56);
  char dateBuffer[10];
  snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d", ti->tm_mday, ti->tm_mon + 1);
  display.print(dateBuffer);

}

void drawPomodoro() {
  unsigned long elapsed = (millis() - timerStartTime) / 1000;
  if (elapsed >= timerDuration) {
    if (currentMode == FOCUS) {
      currentMode = BREAK;
      timerDuration = 5 * 60; 
      timerStartTime = millis();
      triggerBuzzer(2);
    } else {
      saveSession();
      currentMode = CLOCK;
      triggerBuzzer(3);
    }
    return;
  }

  long remaining = timerDuration - elapsed;
  int mins = remaining / 60;
  int secs = remaining % 60;

  display.setFont();
  display.setCursor(0, 0);
  display.print(currentMode == FOCUS ? "FOCUSING" : "BREAK TIME");

  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(25, 40);
  char timerBuf[10];
  snprintf(timerBuf, sizeof(timerBuf), "%02d:%02d", mins, secs);
  display.print(timerBuf);
  int progressWidth = map(elapsed, 0, timerDuration, 0, 128);
  display.drawRect(0, 55, 128, 7, SSD1306_WHITE);
  display.fillRect(0, 55, progressWidth, 7, SSD1306_WHITE);
}