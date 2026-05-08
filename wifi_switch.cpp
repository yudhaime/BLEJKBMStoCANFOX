#include "wifi_switch.h"
#include "config.h"

bool wifiEnabled = false;
bool keepWiFiOn = false;

void initWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  wifiEnabled = true;
}

void disableWiFi() {
  if (wifiEnabled && !keepWiFiOn) {
    Serial.println("🔵 WiFi OFF (free resources)");
    WiFi.mode(WIFI_OFF);
    wifiEnabled = false;
  }
}

void enableWiFiIfDisconnected(bool bmsConnected) {

  // ================= ADD FEATURE ONLY =================
  if (keepWiFiOn) {
    if (!wifiEnabled) {
      Serial.println("⚪ WiFi forced ON (keepWiFi enabled)");
      initWiFiAP();
    }
    return;
  }

  // ================= ORIGINAL BEHAVIOR =================
  if (bmsConnected) {
    disableWiFi();
  } else {
    if (!wifiEnabled) {
      Serial.println("⚪ WiFi ON (BMS disconnected)");
      initWiFiAP();
    }
  }
}
