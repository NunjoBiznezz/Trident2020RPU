/**************************************************************************
 * MachineEepromMode.h
 *
 * Machine EEPROM settings mode: score levels, high score, credits, audit
 * counters, and coin-per-credit selectors.
 * Setting numbering is local to this mode (1 = score level 1 … 15 = CPC chute 3).
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "PinballMachine.h"
#include <stdint.h>

class MachineEepromMode : public MachineMode {
   static constexpr uint8_t kEepromScoreLevel1  =  1;
   static constexpr uint8_t kEepromScoreLevel2  =  2;
   static constexpr uint8_t kEepromScoreLevel3  =  3;
   static constexpr uint8_t kEepromHighScore    =  4;
   static constexpr uint8_t kEepromCredits      =  5;
   static constexpr uint8_t kEepromTotalPlays   =  6;
   static constexpr uint8_t kEepromTotalReplays =  7;
   static constexpr uint8_t kEepromHiscrBeat    =  8;
   static constexpr uint8_t kEepromChute2Coins  =  9;
   static constexpr uint8_t kEepromChute1Coins  = 10;
   static constexpr uint8_t kEepromChute3Coins  = 11;
   static constexpr uint8_t kEepromBoot         = 12;
   static constexpr uint8_t kEepromCpcChute1    = 13;
   static constexpr uint8_t kEepromCpcChute2    = 14;
   static constexpr uint8_t kEepromCpcChute3    = 15;

   uint8_t internalState_ = 0;
   bool    stateChanged_  = false;

   PinballMachine* machine_ = nullptr;

   unsigned long savedValue_            = 0;
   unsigned long resetHold_             = 0;
   unsigned long nextSpeedyValueChange_ = 0;
   unsigned long numSpeedyChanges_      = 0;
   unsigned long lastResetPress_        = 0;

public:
   void setDependencies(PinballMachine& machine) {
      machine_ = &machine;
   }

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
