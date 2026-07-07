/**************************************************************************
 * Trident2020AdjustmentsMode.h
 *
 * Operator adjustment menu: the 14 game settings stored in EEPROM.
 * Entered from MachineEepromMode when the operator cycles past the last
 * EEPROM setting; exits to AttractMode when the operator cycles past
 * the last adjustment or hits the slam switch.
 * Adjustment numbering is local to this mode (1 = free play … 14 = dim level).
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "MachineSettings.h"
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

class Trident2020AdjustmentsMode : public MachineMode {
   static constexpr uint8_t kAdjFreeplay          =  1;  // Free Play -> On/Off
   static constexpr uint8_t kAdjBallSave          =  2;  // Ball save time in ms (1000)
   static constexpr uint8_t kAdjSfxAndSoundtrack  =  3;
   static constexpr uint8_t kAdjMusicVolume       =  4;
   static constexpr uint8_t kAdjSfxVolume         =  5;
   static constexpr uint8_t kAdjCalloutsVolume    =  6;
   static constexpr uint8_t kAdjTournamentScoring =  7;
   static constexpr uint8_t kAdjTiltWarning       =  8;
   static constexpr uint8_t kAdjAwardOverride     =  9;
   static constexpr uint8_t kAdjBallsOverride     = 10;
   static constexpr uint8_t kAdjScrollingScores   = 11;
   static constexpr uint8_t kAdjExtraBallAward    = 12;
   static constexpr uint8_t kAdjSpecialAward      = 13;
   static constexpr uint8_t kAdjDimLevel          = 14;
   static constexpr uint8_t kAdjDone              = 15;

   uint8_t internalState_ = 0;
   bool    stateChanged_  = false;

   Trident2020Game* game_     = nullptr;
   PinballMachine*  machine_  = nullptr;
   MachineSettings* settings_ = nullptr;

   uint8_t        adjustmentType_              = 0;
   uint8_t        numAdjustmentValues_         = 0;
   uint8_t        adjustmentValues_[8]         = {};
   unsigned long  adjustmentScore_             = 0;
   uint8_t*       currentAdjustmentByte_       = nullptr;
   unsigned long* currentAdjustmentUL_         = nullptr;
   uint8_t        currentAdjustmentStorageByte_ = 0;
   uint8_t        tempValue_                   = 0;
   unsigned long  soundSettingTimeout_         = 0;

public:
   void setDependencies(Trident2020Game& game, PinballMachine& machine, MachineSettings& s) {
      game_     = &game;
      machine_  = &machine;
      settings_ = &s;
   }

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;

protected:
   void enterSubstate(uint8_t subState, unsigned long currentTime);
};
