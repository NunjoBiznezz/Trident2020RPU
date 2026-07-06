/**************************************************************************
 * SelfTestMode.h
 *
 * Operator self-test and adjustment menu. Knows concretely about
 * TridentMachine so it can reach the pointer getters for audio
 * adjustment variables. All per-session state is held as members so
 * that re-entering self-test starts fresh.
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "MachineState.h"
#include "Trident2020Game.h"
#include "TridentMachine.h"
#include <stdint.h>

// Non-const pointers to the operator-adjustable game/machine settings
// stored in main.cpp. Passed in once via setDependencies().
struct SelfTestSettings {
   bool*          freePlayMode;
   uint8_t*       ballSaveNumSeconds;
   bool*          tournamentScoring;
   uint8_t*       maxTiltWarnings;
   uint8_t*       scoreAwardReplay;
   uint8_t*       ballsPerGame;
   bool*          scrollingScores;
   unsigned long* extraBallValue;
   unsigned long* specialValue;
   uint8_t*       dimLevel;
};

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

   Trident2020Game*   game_    = nullptr;
   TridentMachine* machine_ = nullptr;
   SelfTestSettings   settings_ = {};

   // Operator adjustment state (was scattered as main.cpp globals)
   uint8_t       adjustmentType_             = 0;
   uint8_t       numAdjustmentValues_        = 0;
   uint8_t       adjustmentValues_[8]        = {};
   unsigned long adjustmentScore_            = 0;
   uint8_t*      currentAdjustmentByte_      = nullptr;
   unsigned long* currentAdjustmentUL_       = nullptr;
   uint8_t       currentAdjustmentStorageByte_ = 0;
   uint8_t       tempValue_                  = 0;
   unsigned long soundSettingTimeout_        = 0;

   // Called on exit to re-apply all stored parameters.
   void (*readParamsCallback_)() = nullptr;

public:
   void setDependencies(Trident2020Game& game, TridentMachine& machine,
                        SelfTestSettings settings) {
      game_     = &game;
      machine_  = &machine;
      settings_ = settings;
   }

   void setReadParamsCallback(void (*cb)()) { readParamsCallback_ = cb; }

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
