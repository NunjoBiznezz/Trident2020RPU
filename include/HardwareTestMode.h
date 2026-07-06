/**************************************************************************
 * HardwareTestMode.h
 *
 * Hardware self-test mode: lamp, display, solenoid, switch, and sound
 * tests plus the CPC coin-per-credit selectors.  Entered via the service
 * switch from AttractMode; transitions to AdjustmentsMode when the
 * operator cycles past the last test, or back to AttractMode on slam.
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "PinballMachine.h"
#include <stdint.h>

class HardwareTestMode : public MachineMode {
   int  internalState_ = 0;
   bool stateChanged_  = false;

   PinballMachine* machine_ = nullptr;

   unsigned long lastSolTestTime_       = 0;
   unsigned long savedValue_            = 0;
   unsigned long resetHold_             = 0;
   unsigned long nextSpeedyValueChange_ = 0;
   unsigned long numSpeedyChanges_      = 0;
   unsigned long lastResetPress_        = 0;
   uint8_t       curValue_              = 0;
   uint8_t       soundPlaying_          = 0;
   uint8_t       soundToPlay_           = 0;
   bool          solenoidCycle_         = true;

public:
   void setDependencies(PinballMachine& machine) {
      machine_ = &machine;
   }

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
