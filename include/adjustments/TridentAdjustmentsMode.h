/**************************************************************************
 * adjustments/TridentAdjustmentsMode.h
 *
 * Operator adjustment menu for the Original Trident rule-set settings.
 * Setting numbers, EEPROM addresses, and adjustment objects are all
 * private to TridentAdjustmentsMode.cpp.
 **************************************************************************/

#pragma once
#include "../MachineMode.h"
#include "../PinballMachine.h"
#include <stdint.h>

class TridentAdjustmentsMode : public MachineMode {
   uint8_t internalState_ = 0;
   bool    stateChanged_  = false;

   PinballMachine* machine_ = nullptr;

   unsigned long selfTestLastPressedTime_ = 0;

public:
   void setDependencies(PinballMachine& machine);

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
