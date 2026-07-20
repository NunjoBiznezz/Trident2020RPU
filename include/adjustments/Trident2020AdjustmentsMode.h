/**************************************************************************
 * Trident2020AdjustmentsMode.h
 *
 * Operator adjustment menu for the Trident 2020 game settings.
 * Setting numbers, EEPROM addresses, and adjustment objects are all
 * private to Trident2020AdjustmentsMode.cpp.
 **************************************************************************/

#pragma once
#include "GameAdjustmentsMode.h"
#include "../Trident2020Game.h"
#include <stdint.h>

class Trident2020AdjustmentsMode : public GameAdjustmentsMode {
   Trident2020Game* game_ = nullptr;

   uint8_t           adjustmentCount() const override;
   StoredAdjustment* getAdjustment(uint8_t index) override;
   TopState          activeState()     const override;
   TopState          completedState()  const override;
   uint8_t           modeId()          const override { return 20; }
   void              onUpdate()              override;
   void              onExit()               override;

public:
   void setDependencies(Trident2020Game& game, PinballMachine& machine);
};
