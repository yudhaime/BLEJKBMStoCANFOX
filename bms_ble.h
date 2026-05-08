#pragma once
#include <NimBLEDevice.h>
#include <vector>

// ================= BMS DATA =================
extern float socPercent;
extern float totalVoltage;
extern float cellVoltage[24];
extern bool socChanged;

// ================= STATUS =================
extern bool bmsConnected;

// ================= BLE SCAN STRUCT =================
struct BLEDeviceInfo {
  String address;
  String name;
  int rssi;
};

extern std::vector<BLEDeviceInfo> lastScanResults;

// ================= GLOBAL SCAN =================
extern NimBLEScan* pScan;

// ================= BLE FUNCTIONS =================
void initBMS();
void handleBMS();

// ================= SCAN FUNCTIONS =================
bool startBLEScan(int durationSeconds);
void clearScanResults();

// ================= JK BMS CLASS =================
class JKBMS {
public:
  JKBMS(const std::string& mac);

  bool connectToServer();
  void handleNotification(uint8_t* pData, size_t length);

  // ================= DATA =================
  float cellVoltage[24] = {0};
  float Battery_Voltage = 0;
  float Charge_Current = 0;
  int Percent_Remain = 0;

  // ================= STATUS =================
  bool connected = false;
  std::string targetMAC;

  // ================= BLE =================
  const NimBLEAdvertisedDevice* advDevice = nullptr;
  bool doConnect = false;

private:
  NimBLERemoteCharacteristic* pChr = nullptr;
};
