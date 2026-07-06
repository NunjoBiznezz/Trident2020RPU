/**************************************************************************
 * AttractMode.h
 *
 * Top-level attract mode state. Cycles lamp displays and waits for the
 * start button; transitions to Game when a player is added.
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "PinballMachine.h"
#include "Trident2020Game.h"
#include <stdint.h>

class AttractMode : public MachineMode {
   unsigned long lastLadderTime_    = 0;
   uint8_t       lastLadderBonus_   = 0;
   unsigned long lastStarTime_      = 0;
   uint8_t       lastHeadMode_      = 255;
   uint8_t       lastPlayfieldMode_ = 255;

   Trident2020Game* game_    = nullptr;
   PinballMachine*  machine_ = nullptr;
   GameContext*     ctx_     = nullptr;

public:
   void setDependencies(Trident2020Game& game, PinballMachine& machine, GameContext& ctx) {
      game_    = &game;
      machine_ = &machine;
      ctx_     = &ctx;
   }

   void     enter(unsigned long currentTime) override;
   TopState update(unsigned long currentTime) override;
};
