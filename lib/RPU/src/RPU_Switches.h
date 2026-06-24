#pragma once

// Forward declarations of internal RPU bus functions defined in RPU.cpp
void RPU_DataWrite(int address, uint8_t data);
uint8_t RPU_DataRead(int address);

constexpr unsigned NUM_SWITCH_BYTES = 5;
constexpr unsigned NUM_SWITCH_BYTES_ON_U10_PORT_A = 5;
constexpr unsigned MAX_NUM_SWITCHES = 40;

void RPU_ResetSwitchState();
void RPU_ServiceSwitchBank(uint8_t switchCount);
