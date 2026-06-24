#pragma once

// Forward declaration of internal RPU bus function defined in RPU.cpp
void RPU_DataWrite(int address, uint8_t data);

void RPU_ResetLampState();
void RPU_StrobeLamps();
