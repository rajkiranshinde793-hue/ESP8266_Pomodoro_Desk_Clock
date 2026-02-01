#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Hardware Pins 
#define BUTTON_PIN D5
#define BUZZER_PIN D6

// OLED display settings 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Constants 
#define EEPROM_ADDR 0        
#define MAX_SESSIONS 24      
#define LONG_PRESS_MS 500
#define SYNC_INTERVAL 1800000 // 30 Minutes

// NTP Settings 
#define NTP_OFFSET 19800 // GMT +5:30

// Enums (Must be here so main.cpp understands them) 
enum Mode { CLOCK, FOCUS, BREAK };
enum NetworkState { NET_IDLE, NET_CONNECTING, NET_UPDATING };

#endif