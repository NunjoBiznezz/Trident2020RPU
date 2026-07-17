/**************************************************************************
 * AttractMode.h
 *
 * Top-level attract mode state. Cycles lamp displays and waits for the
 * start button; transitions to Game when a player is added.
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "MachineSettings.h"
#include "PinballMachine.h"
#include <stdint.h>

class AttractMode : public MachineMode {
   uint8_t          lastHeadMode_      = 255;
   uint8_t          lastPlayfieldMode_ = 255;
   uint8_t          animNum_           = 0;
   uint8_t          animStep_          = 0;
   uint8_t          animCount_         = 0; // Repeat animations 3 times
   unsigned long    lastAnimFrameTime_ = 0;
   unsigned long    savedScores_[4]    = {};
   uint8_t          savedNumPlayers_   = 0;
   unsigned long    enterTime_               = 0;
   unsigned long    selfTestLastPressedTime_ = 0;

   PinballMachine*  machine_  = nullptr;

public:
   void setDependencies(PinballMachine& machine) {
      machine_  = &machine;
   }

   void     enter(unsigned long currentTime) override;
   TopState update(unsigned long currentTime) override;
};
