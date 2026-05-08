#include <Arduino.h>
#include "can_handler.h"
#include "bms_ble.h"
#include "web_config.h"
#include "wifi_switch.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("JK BMS BLE -> CAN Bridge");

  initCAN();
  initBMS();
  initWebServer(); // Start web server initially
}

void loop() {
  handleBMS();
  handleCAN();
  
  enableWiFiIfDisconnected(bmsConnected);
  
  if (wifiEnabled) {
    handleWebServer();
  }

  delay(10);
}
