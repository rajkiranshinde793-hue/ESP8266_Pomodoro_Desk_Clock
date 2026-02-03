#include "GoogleLogger.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// 1. INCLUDE YOUR WIFI CREDENTIALS
#include "wifi_cred.h" 

// 2. PASTE YOUR WEB APP URL HERE (Make sure it ends in /exec)
const char* GOOGLE_SCRIPT_URL = "Paste your Web Url"; 

void ensureWiFiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    Serial.println("[Logger] Waking WiFi...");
    
    // Force wake RF hardware
    WiFi.forceSleepWake();
    delay(1);
    WiFi.mode(WIFI_STA);
    
    // 3. EXPLICITLY CONNECT (This was missing!)
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 40) { // Wait 20 seconds max
        delay(500);
        Serial.print(".");
        retries++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[Logger] Connected! IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("[Logger] Connection FAILED.");
    }
}

void logToGoogle(String type, int durationMinutes) {
    Serial.println("[Logger] Starting Log Sequence...");
    
    ensureWiFiConnected();

    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure(); // Ignore SSL certificate (Required)

        HTTPClient http;
        Serial.print("[Logger] Sending data to: ");
        Serial.println(GOOGLE_SCRIPT_URL);

        // Follow Redirects is CRITICAL
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

        if (http.begin(client, GOOGLE_SCRIPT_URL)) {
            http.addHeader("Content-Type", "application/json");

            String payload = "{\"type\":\"" + type + "\", \"duration\":\"" + String(durationMinutes) + "\"}";
            Serial.println("[Logger] Payload: " + payload);

            int httpCode = http.POST(payload);

            if (httpCode > 0) {
                Serial.printf("[Logger] HTTP Status: %d\n", httpCode);
                String response = http.getString();
                Serial.println("[Logger] Server Response: " + response);
            } else {
                Serial.printf("[Logger] HTTP Failed. Error: %s\n", http.errorToString(httpCode).c_str());
            }
            http.end();
        } else {
            Serial.println("[Logger] Unable to begin HTTP connection.");
        }
    } else {
        Serial.println("[Logger] Error: WiFi not connected, cannot log.");
    }
}