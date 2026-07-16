/**************************************************************************
 * StoredAdjustmentsMode.h
 *
 * Machine EEPROM settings mode: score levels, high score, credits, audit
 * counters, and coin-per-credit selectors.  All data is stored in the
 * Arduino's own EEPROM (via EEPROM.h) at addresses 148–191, independent
 * of the RPU library's reserved address space (0–52).
 *
 * Setting numbering, EEPROM addresses, and adjustment objects are all
 * private to StoredAdjustmentsMode.cpp.
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "PinballMachine.h"
#include <stdint.h>

class StoredAdjustmentsMode : public MachineMode {
   uint8_t internalState_ = 0;
   bool    stateChanged_  = false;

   PinballMachine* machine_ = nullptr;

   unsigned long resetHold_               = 0;
   unsigned long lastResetPress_          = 0;
   unsigned long selfTestLastPressedTime_ = 0;

public:
   void setDependencies(PinballMachine& machine);

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
