#pragma once
#include <driver/twai.h>

// WiFi Configuration
#define WIFI_AP_SSID       "JK-BMS-Setup"
#define WIFI_AP_PASSWORD   "12345678"

// CAN Configuration
#define CAN_TX_GPIO        21
#define CAN_RX_GPIO        22

// BLE Configuration
#define BLE_SCAN_DURATION      10000  // 10 seconds for device discovery
#define BLE_SCAN_INTERVAL      10000  // 10 seconds between scans when disconnected

// CAN Message IDs
#define CAN_ID_SOC         0x0A6E0D09
#define CAN_ID_VOLTAGE     0x0A6D0D09  
#define CAN_ID_CELLS       0x0A6F0D09
#define CAN_ID_HW_INFO     0x0A740D09
#define CAN_ID_HW_VERSION  0x0A750D09
#define CAN_ID_FW_VERSION  0x0A760D09
#define CAN_ID_EMPTY       0x0AB40D09

// Web Configuration
#define WEB_SERVER_PORT    80

// BMS Configuration
#define MAX_CELLS          24
#define MAX_CELLS_TO_SEND  8
#define SOC_CHANGE_THRESHOLD 0.5f

// Hardware/Software Info
#define HW_INFO_STRING     "BWBM-625-23S-100A-01K3224-E1"
#define HW_VERSION_STRING  "H:v21"
#define FW_VERSION_STRING  "F:v23"
