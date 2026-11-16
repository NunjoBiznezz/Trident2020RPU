/**************************************************************************
 * adjustments/TridentAdjustmentsMode.h
 *
 * Operator adjustment menu for the Original Trident rule-set settings.
 * Setting numbers, EEPROM addresses, and adjustment objects are all
 * private to TridentAdjustmentsMode.cpp.
 **************************************************************************/

#pragma once
#include "GameAdjustmentsMode.h"
#include "../TridentGame.h"
#include <stdint.h>

class TridentAdjustmentsMode : public GameAdjustmentsMode {
   TridentGame* game_ = nullptr;

   uint8_t           adjustmentCount() const override;
   StoredAdjustment* getAdjustment(uint8_t index) override;
   TopState          activeState()     const override;
   TopState          completedState()  const override;
   uint8_t           modeId()          const override { return 1; }
   void              onExit()                override;

public:
   void setDependencies(TridentGame& game, PinballMachine& machine);
};
