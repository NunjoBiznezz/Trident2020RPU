/**************************************************************************
 * SelfTestMode.h
 *
 * Operator self-test and adjustment menu. Holds a MachineSettings* for
 * direct field access during adjustments and a PinballMachine* for audio
 * control. All per-session state is held as members so that re-entering
 * self-test starts fresh.
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "MachineSettings.h"
#include "MachineState.h"
#include "PinballMachine.h"
#include "Trident2020Game.h"
#include <stdint.h>

enum AdjustmentType_t : uint8_t {
   ADJ_TYPE_LIST               = 1,
   ADJ_TYPE_MIN_MAX            = 2,
   ADJ_TYPE_MIN_MAX_DEFAULT    = 3,
   ADJ_TYPE_SCORE              = 4,
   ADJ_TYPE_SCORE_WITH_DEFAULT = 5,
   ADJ_TYPE_SCORE_NO_DEFAULT   = 6
};

class SelfTestMode : public MachineMode {
   int  internalState_ = MACHINE_STATE_TEST_LAMPS;
   bool stateChanged_  = false;

   Trident2020Game* game_     = nullptr;
   PinballMachine*  machine_  = nullptr;
   MachineSettings* settings_ = nullptr;

   // Operator adjustment state (was scattered as main.cpp globals)
   uint8_t       adjustmentType_              = 0;
   uint8_t       numAdjustmentValues_         = 0;
   uint8_t       adjustmentValues_[8]         = {};
   unsigned long adjustmentScore_             = 0;
   uint8_t*      currentAdjustmentByte_       = nullptr;
   unsigned long* currentAdjustmentUL_        = nullptr;
   uint8_t       currentAdjustmentStorageByte_ = 0;
   uint8_t       tempValue_                   = 0;
   unsigned long soundSettingTimeout_         = 0;

public:
   void setDependencies(Trident2020Game& game, PinballMachine& machine, MachineSettings& s) {
      game_     = &game;
      machine_  = &machine;
      settings_ = &s;
   }

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
