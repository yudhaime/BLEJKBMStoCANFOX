#pragma once
#include <WiFi.h>

extern bool wifiEnabled;
extern bool keepWiFiOn;

void initWiFiAP();
void disableWiFi();
void enableWiFiIfDisconnected(bool bmsConnected);
