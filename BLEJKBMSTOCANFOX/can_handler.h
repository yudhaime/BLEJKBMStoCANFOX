#pragma once
#include <driver/twai.h>

void initCAN();
void sendCANFrame(uint32_t canId, uint8_t* payload, uint8_t len);
void handleCAN();
