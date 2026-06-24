#pragma once
#include "RPU_config.h"

// Forward declarations of internal RPU bus functions defined in RPU.cpp
void RPU_DataWrite(int address, uint8_t data);
uint8_t RPU_DataRead(int address);

class DipSwitchManager {
public:
   void read();
   uint8_t get(uint8_t index) const;

private:
#ifdef RPU_OS_USE_DIP_SWITCHES
   uint8_t dipSwitches_[4] = {};
#endif
};

extern DipSwitchManager dipSwitches;
