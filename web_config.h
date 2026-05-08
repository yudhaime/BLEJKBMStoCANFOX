#pragma once
#include <WiFi.h>

// ===== GLOBAL (dipakai BLE scan realtime) =====
extern WiFiClient eventClient;
extern bool clientConnected;

// ===== WEB =====
void initWebServer();
void handleWebServer();
