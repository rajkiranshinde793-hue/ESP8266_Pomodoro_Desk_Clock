#pragma once
#ifndef GOOGLE_LOGGER_H
#define GOOGLE_LOGGER_H

#include <Arduino.h>

// Declare the function so main.cpp knows it exists
void logToGoogle(String type, int durationMinutes);

#endif