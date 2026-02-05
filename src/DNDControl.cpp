#include "DNDControl.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "credentials.h" 



enum DNDState { DND_IDLE, DND_WAKING_WIFI, DND_WAITING_FOR_WIFI, DND_SENDING };
DNDState dndState = DND_IDLE;

bool targetDndEnable = false;
unsigned long dndWifiStartTime = 0;

// 1. THE TRIGGER (Runs instantly)
void queueDNDChange(bool enable) {
    targetDndEnable = enable;
    dndState = DND_WAKING_WIFI; // Start the machine
}

// 2. THE BACKGROUND WORKER (Runs in loop)
void handleDNDBackground() {
    switch (dndState) {
        
        case DND_IDLE:
            // Do nothing, just waiting for a command
            break;

        case DND_WAKING_WIFI:
            // If WiFi is already connected, skip to sending
            if (WiFi.status() == WL_CONNECTED) {
                dndState = DND_SENDING;
            } else {
                // Wake up WiFi (Fire and Forget)
                WiFi.forceSleepWake();
                delay(1);
                WiFi.mode(WIFI_STA);
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                
                dndWifiStartTime = millis();
                dndState = DND_WAITING_FOR_WIFI; // Move to next state
            }
            break;

        case DND_WAITING_FOR_WIFI:
            // Check status, but DO NOT PAUSE (No blocking loops!)
            if (WiFi.status() == WL_CONNECTED) {
                dndState = DND_SENDING;
            } 
            else if (millis() - dndWifiStartTime > 15000) {
                // Timeout after 15 seconds
                Serial.println("[DND] Background Error: WiFi Timeout");
                dndState = DND_IDLE; // Give up
            }
            break;

        case DND_SENDING:
            // Now we send the request (This takes ~200ms, which is acceptable)
            if (WiFi.status() == WL_CONNECTED) {
                WiFiClientSecure client;
                client.setInsecure();
                HTTPClient http;
                
                const char* targetURL = targetDndEnable ? URL_DND_ON : URL_DND_OFF;
                
                Serial.println("[DND] Background: Sending Webhook...");
                
                http.begin(client, targetURL);
                int code = http.GET();
                http.end();
                
                Serial.printf("[DND] Result: %d\n", code);
            }
            dndState = DND_IDLE; // Job Done!
            break;
    }
}