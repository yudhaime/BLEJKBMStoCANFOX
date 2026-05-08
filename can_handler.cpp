#include "can_handler.h"
#include <Arduino.h>
#include "config.h"

extern float socPercent, totalVoltage;
extern float cellVoltage[MAX_CELLS];
extern bool socChanged;
extern bool bmsConnected;

// CAN Configuration
static unsigned long lastSendTime = 0;
const unsigned long CAN_INTERVAL = 500; // Send every 500ms

// Safety fallback values
static float lastValidSoc = 25.0;        // MINIMAL 25% untuk safety
static float lastValidVoltage = 72.0;    // Default voltage normal  
static bool firstDataReceived = false;   // Flag data pertama

twai_message_t msg;

// ================= TABEL LOOKUP SOC (AKURASI TINGGI) =================
// Mapping dari raw value (yang diterima speedometer) ke persen
// index = persen (0-100), value = raw yang harus dikirim
const uint16_t percentToRaw[101] = {
  0, 60,70,80,90,95,105,115,125,135,140,150,160,170,180,185,195,205,215,225,
  230,240,250,260,270,275,285,295,305,315,320,330,340,350,360,365,375,385,395,405,
  410,420,430,440,450,455,465,475,485,495,500,510,520,530,540,550,555,565,575,585,
  590,600,610,620,630,635,645,655,665,675,680,690,700,710,720,725,735,745,755,765,
  770,780,790,800,810,815,825,835,845,855,860,870,880,890,900,905,915,925,935,945,950
};

// Konversi dari persen (0-100) ke raw value untuk dikirim ke CAN
uint16_t percentToRawValue(float percent) {
  int p = (int)percent;
  if (p < 0) p = 0;
  if (p > 100) p = 100;
  
  // Cari di tabel berdasarkan persen integer
  if (p == 100) return percentToRaw[100];
  if (p == 0) return percentToRaw[0];
  
  // Interpolasi linear untuk persen yang tidak bulat
  float rawLow = (float)percentToRaw[p];
  float rawHigh = (float)percentToRaw[p + 1];
  float fraction = percent - (float)p;
  float rawValue = rawLow + (fraction * (rawHigh - rawLow));
  
  return (uint16_t)rawValue;
}

void initCAN() {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_GPIO, (gpio_num_t)CAN_RX_GPIO, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  
  esp_err_t result = twai_driver_install(&g_config, &t_config, &f_config);
  if (result == ESP_OK) {
    result = twai_start();
    if (result == ESP_OK) {
      Serial.println("CAN Started successfully");
      return;
    }
  }
  
  Serial.printf("CAN Init failed: %d\n", result);
}

void sendCANFrame(uint32_t canId, uint8_t* payload, uint8_t len) {
  msg.identifier = canId;
  msg.extd = true;
  msg.data_length_code = len;
  memcpy(msg.data, payload, len);
  
  // SUPPRESS ERROR MESSAGES
  twai_transmit(&msg, pdMS_TO_TICKS(100));
}

void handleCAN() {
  // Update values based on BMS connection
  if (bmsConnected && socPercent > 0) {
    // Data real dari BMS - update (bisa di bawah 25% jika memang real)
    lastValidSoc = socPercent;
    lastValidVoltage = totalVoltage;
    firstDataReceived = true;
  } else {
    // BMS disconnected atau belum ada data - PASTIKAN minimal 25%
    lastValidSoc = 25.0;
    // Voltage tetap hold nilai terakhir
  }
  
  if (millis() - lastSendTime > CAN_INTERVAL) {
    lastSendTime = millis();

    // SOC frame - menggunakan lookup table untuk akurasi
    uint8_t socPayload[8] = {0};
    uint16_t socRawValue = percentToRawValue(lastValidSoc);
    socPayload[0] = socRawValue >> 8;
    socPayload[1] = socRawValue & 0xFF;
    sendCANFrame(CAN_ID_SOC, socPayload, 8);

    // Voltage frame - selalu kirim
    uint16_t vpack = (uint16_t)(lastValidVoltage * 10); // 0.1V resolution
    uint8_t vPayload[8] = {vpack >> 8, vpack & 0xFF, 0, 0, 0, 0, 0, 0};
    sendCANFrame(CAN_ID_VOLTAGE, vPayload, 8);

    // Cell voltages (up to 8 cells) - selalu kirim
    uint8_t cellPayload[8] = {0};
    for(int i = 0; i < MAX_CELLS_TO_SEND; i++) {
      if(cellVoltage[i] > 0) {
        uint16_t cv = (uint16_t)(cellVoltage[i] * 1000); // Convert to mV
        cellPayload[i * 2] = cv >> 8;
        cellPayload[i * 2 + 1] = cv & 0xFF;
      }
    }
    sendCANFrame(CAN_ID_CELLS, cellPayload, 8);

    // Debug info
    if (!firstDataReceived) {
      Serial.printf("CAN: Waiting BMS - SOC: 25.0%% (Safety Minimum)\n");
    } else if (!bmsConnected) {
      Serial.printf("CAN: BMS Disconnected - SOC: 25.0%% (Safety Minimum)\n");
    } else {
      uint16_t rawCheck = percentToRawValue(lastValidSoc);
      Serial.printf("CAN: BMS SOC: %.1f%% -> Raw: %d (0x%04X)\n", 
                    lastValidSoc, rawCheck, rawCheck);
    }

    socChanged = false;
  }
}
