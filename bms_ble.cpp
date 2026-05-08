#include "bms_ble.h"
#include "web_config.h"
#include <WiFi.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <vector>

// ================= EXTERN (SSE) =================
extern WiFiClient eventClient;
extern bool clientConnected;

// ================= DATA =================
float socPercent = 0;
float totalVoltage = 0;
float cellVoltage[24] = {0};
bool socChanged = true;
bool bmsConnected = false;

// ================= GLOBAL =================
JKBMS* jkBMS = nullptr;
NimBLEScan* pScan;

std::vector<BLEDeviceInfo> lastScanResults;

// ================= HELPER NAMA =================
String getDeviceName(const NimBLEAdvertisedDevice* d) {
  std::string name = d->getName();

  if (name.length() == 0 && d->haveName()) {
    name = d->getName();
  }

  if (name.length() == 0 && d->haveManufacturerData()) {
    name = "BLE Device";
  }

  if (name.length() == 0) {
    name = "Unknown";
  }

  return String(name.c_str());
}

// ================= SCAN CALLBACK (WEB) =================
class ScanResultsCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) {

    BLEDeviceInfo device;
    device.address = advertisedDevice->getAddress().toString().c_str();
    device.name = getDeviceName(advertisedDevice);
    device.rssi = advertisedDevice->getRSSI();

    lastScanResults.push_back(device);

    Serial.printf("Found: %s | %s | RSSI: %d\n",
                  device.name.c_str(),
                  device.address.c_str(),
                  device.rssi);

    if (clientConnected && eventClient.connected()) {
      String json = "{";
      json += "\"address\":\"" + device.address + "\",";
      json += "\"name\":\"" + device.name + "\",";
      json += "\"rssi\":" + String(device.rssi);
      json += "}";

      eventClient.print("data: ");
      eventClient.print(json);
      eventClient.print("\n\n");
    }
  }
};

// ================= SCAN CALLBACK (AUTO CONNECT) =================
class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
    if (jkBMS && !jkBMS->connected && !jkBMS->doConnect) {
      if (advertisedDevice->getAddress().toString() == jkBMS->targetMAC) {
        Serial.printf(">>> TARGET FOUND: %s\n", jkBMS->targetMAC.c_str());
        jkBMS->advDevice = advertisedDevice;
        jkBMS->doConnect = true;
        NimBLEDevice::getScan()->stop();
      }
    }
  }
};

// ================= NOTIFY =================
void notifyCB(NimBLERemoteCharacteristic* pChr, uint8_t* pData, size_t length, bool isNotify) {
  if (jkBMS) {
    jkBMS->handleNotification(pData, length);
  }
}

// ================= JKBMS =================
JKBMS::JKBMS(const std::string& mac) : targetMAC(mac) {}

bool JKBMS::connectToServer() {
  if (!advDevice) {
    Serial.println("No device to connect");
    return false;
  }

  Serial.printf("Connecting to %s...\n", targetMAC.c_str());

  NimBLEClient* pClient = NimBLEDevice::createClient();
  pClient->setConnectionParams(12, 12, 0, 150);
  pClient->setConnectTimeout(5000);

  if (!pClient->connect(advDevice)) {
    Serial.println("Connection failed");
    return false;
  }

  Serial.println("Connected!");

  NimBLERemoteService* pSvc = pClient->getService("ffe0");
  if (!pSvc) {
    Serial.println("Service not found");
    return false;
  }

  NimBLERemoteCharacteristic* pChr = pSvc->getCharacteristic("ffe1");
  if (!pChr) {
    Serial.println("Characteristic not found");
    return false;
  }

  if (pChr->canNotify()) {
    pChr->subscribe(true, notifyCB);
    Serial.println("Subscribed to BMS notifications");
  }

  connected = true;
  bmsConnected = true;

  return true;
}

void JKBMS::handleNotification(uint8_t* pData, size_t length) {
  if (length < 7) return;

  totalVoltage = (pData[4] << 8 | pData[5]) * 0.01;
  socPercent = pData[6];

  socChanged = true;

  Serial.printf("Voltage: %.2f | SOC: %.1f\n", totalVoltage, socPercent);
}

// ================= INIT =================
void initBMS() {
  Serial.println("Init BLE...");

  Preferences prefs;
  prefs.begin("bms_config", true);
  String mac = prefs.getString("bms_mac", "");
  prefs.end();

  NimBLEDevice::init("JK-BMS");
  NimBLEDevice::setPower(ESP_PWR_LVL_P3);

  jkBMS = new JKBMS(mac.c_str());

  pScan = NimBLEDevice::getScan();

  pScan->setActiveScan(true);   // ✅ penting
  pScan->setInterval(200);
  pScan->setWindow(120);
  pScan->setMaxResults(0);
}

// ================= HANDLE =================
void handleBMS() {
  if (!jkBMS) return;

  if (jkBMS->doConnect && !jkBMS->connected) {
    jkBMS->connectToServer();
    jkBMS->doConnect = false;
  }

  if (!jkBMS->connected && !pScan->isScanning()) {
    pScan->setScanCallbacks(new ScanCallbacks());
    pScan->start(0, false, true);
  }
}

// ================= SCAN WEB =================
bool startBLEScan(int durationSeconds) {
  clearScanResults();

  if (pScan->isScanning()) {
    pScan->stop();
    delay(50);
  }

  pScan = NimBLEDevice::getScan();

  pScan->setActiveScan(true);   // ✅ penting
  pScan->setInterval(200);
  pScan->setWindow(120);
  pScan->setMaxResults(0);

  pScan->setScanCallbacks(new ScanResultsCallbacks());

  pScan->start(0, false, true);

  return true;
}

void clearScanResults() {
  lastScanResults.clear();
}
