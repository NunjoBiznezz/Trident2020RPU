/**************************************************************************
 * HardwareTestMode.h
 *
 * Hardware self-test mode: lamp, display, solenoid, switch, and sound
 * tests.  Entered via the service switch from AttractMode.
 * Test numbering is local to this mode (1 = LAMPS … 5 = SOUNDS).
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "PinballMachine.h"
#include <stdint.h>

class HardwareTestMode : public MachineMode {
   static constexpr uint8_t kTestLamps    = 1;
   static constexpr uint8_t kTestDisplays = 2;
   static constexpr uint8_t kTestSolenoids = 3;
   static constexpr uint8_t kTestSwitches = 4;
   static constexpr uint8_t kTestSounds   = 5;

   uint8_t internalState_ = 0;
   bool    stateChanged_  = false;

   PinballMachine* machine_ = nullptr;

   unsigned long lastSolTestTime_       = 0;
   unsigned long lastResetPress_        = 0;
   unsigned long selfTestLastPressedTime_ = 0;
   uint8_t       solenoidIndex_   = 0;
   uint8_t       curValue_        = 0;
   uint8_t       soundPlaying_    = 0;
   bool          solenoidCycle_   = true;

public:
   void setDependencies(PinballMachine& machine) {
      machine_ = &machine;
   }

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
