#pragma once

// Forward declaration of internal RPU bus functions defined in RPU.cpp
void RPU_DataWrite(int address, uint8_t data);
uint8_t RPU_DataRead(int address);

void RPU_ReadDipSwitches();
uint8_t RPU_GetDipSwitches(uint8_t index);
