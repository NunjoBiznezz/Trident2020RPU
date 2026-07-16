/**************************************************************************
 * Trident2020AdjustmentsMode.h
 *
 * Operator adjustment menu for the 17 Trident 2020 game settings.
 * Setting numbers, EEPROM addresses, and adjustment objects are all
 * private to Trident2020AdjustmentsMode.cpp.
 **************************************************************************/

#pragma once
#include "../MachineMode.h"
#include "../PinballMachine.h"
#include "../Trident2020Game.h"
#include <stdint.h>

class Trident2020AdjustmentsMode : public MachineMode {
   uint8_t internalState_ = 0;
   bool    stateChanged_  = false;

   Trident2020Game* game_    = nullptr;
   PinballMachine*  machine_ = nullptr;

   unsigned long selfTestLastPressedTime_ = 0;

public:
   void setDependencies(Trident2020Game& game, PinballMachine& machine);

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
