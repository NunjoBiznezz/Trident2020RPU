#pragma once
#include "RPU_config.h"

#ifdef RPU_OS_USE_DIP_SWITCHES

namespace RPU {

class DipSwitchManager {
public:
   void read();
   uint8_t get(uint8_t index) const;

private:
   uint8_t dipSwitches_[4] = {};
};

extern DipSwitchManager dipSwitches;

} // namespace RPU

#endif