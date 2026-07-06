/**************************************************************************
 * AdjustmentsMode.cpp  (file kept as SelfTestMode.cpp for build-system compat)
 **************************************************************************/

#include "SelfTestMode.h"
#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include <EEPROM.h>

// Callouts for adjustment states -21 (ADJUST_FREEPLAY) through -35 (ADJUST_DONE).
// Index 0 = ADJUST_FREEPLAY, index 14 = ADJUST_DONE.
static const uint8_t kAdjustmentCalloutMap[15] = {
   153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 171, 0
};

static const uint8_t kSoundSelectorToCalloutsMap[] = {190, 191, 199, 197, 198, 196};

void AdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = MACHINE_STATE_ADJUST_FREEPLAY;
   stateChanged_  = true;
}

void AdjustmentsMode::exit() {
   machine_->readStoredParameters();
}

TopState AdjustmentsMode::update(unsigned long currentTime) {
   bool curStateChanged = stateChanged_;
   stateChanged_ = false;
   int curState   = internalState_;
   int returnState = curState;

   game_->setNumPlayers(0);

   if (curStateChanged) {
      machine_->stopAllAudio();
      uint8_t callout = kAdjustmentCalloutMap[-curState - 21];
      machine_->playCallout(callout);
      soundSettingTimeout_ = 0;
   } else {
      if (soundSettingTimeout_ != 0 && currentTime > soundSettingTimeout_) {
         soundSettingTimeout_ = 0;
         machine_->stopAllAudio();
      }
   }

   uint8_t curSwitch = machine_->pullFirstFromSwitchStack();

   if (curSwitch == SW_SELF_TEST_SWITCH &&
       (currentTime - machine_->getSelfTestChangedTime()) > 250) {
      machine_->setSelfTestChangedTime(currentTime);
      returnState -= 1;
   }

   if (curSwitch == SW_SLAM) {
      returnState = MACHINE_STATE_ATTRACT;
   }

   if (curStateChanged) {
      for (int count = 0; count < 4; count++) {
         machine_->setDisplay(count, 0);
         machine_->setDisplayBlank(count, 0x00);
      }
      machine_->setDisplayCredits((uint8_t)(MACHINE_STATE_TEST_SOUNDS - curState), true);
      machine_->setDisplayBallInPlay(0, false);
      currentAdjustmentByte_        = nullptr;
      currentAdjustmentUL_          = nullptr;
      currentAdjustmentStorageByte_ = 0;

      adjustmentType_      = ADJ_TYPE_MIN_MAX;
      adjustmentValues_[0] = 0;
      adjustmentValues_[1] = 1;
      tempValue_           = 0;

      switch (curState) {
      case MACHINE_STATE_ADJUST_FREEPLAY:
         currentAdjustmentByte_        = (uint8_t*)&settings_->freePlayMode;
         currentAdjustmentStorageByte_ = EEPROM_FREE_PLAY_BYTE;
         break;

      case MACHINE_STATE_ADJUST_BALL_SAVE:
         adjustmentType_      = ADJ_TYPE_LIST;
         numAdjustmentValues_ = 5;
         adjustmentValues_[1] = 5;
         adjustmentValues_[2] = 10;
         adjustmentValues_[3] = 15;
         adjustmentValues_[4] = 20;
         currentAdjustmentByte_        = &settings_->ballSaveNumSeconds;
         currentAdjustmentStorageByte_ = EEPROM_BALL_SAVE_BYTE;
         break;

      case MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK:
         adjustmentType_      = ADJ_TYPE_MIN_MAX;
         adjustmentValues_[1] = 5;
         currentAdjustmentByte_        = &settings_->soundSelector;
         currentAdjustmentStorageByte_ = EEPROM_SOUND_SELECTOR_BYTE;
         break;

      case MACHINE_STATE_ADJUST_MUSIC_VOLUME:
         adjustmentType_      = ADJ_TYPE_MIN_MAX;
         adjustmentValues_[1] = 10;
         currentAdjustmentByte_        = &settings_->musicVolume;
         currentAdjustmentStorageByte_ = EEPROM_MUSIC_VOLUME_BYTE;
         break;

      case MACHINE_STATE_ADJUST_SFX_VOLUME:
         adjustmentType_      = ADJ_TYPE_MIN_MAX;
         adjustmentValues_[1] = 10;
         currentAdjustmentByte_        = &settings_->sfxVolume;
         currentAdjustmentStorageByte_ = EEPROM_SFX_VOLUME_BYTE;
         break;

      case MACHINE_STATE_ADJUST_CALLOUTS_VOLUME:
         adjustmentType_      = ADJ_TYPE_MIN_MAX;
         adjustmentValues_[1] = 10;
         currentAdjustmentByte_        = &settings_->calloutsVolume;
         currentAdjustmentStorageByte_ = EEPROM_CALLOUTS_VOLUME_BYTE;
         break;

      case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
         currentAdjustmentByte_        = (uint8_t*)&settings_->tournamentScoring;
         currentAdjustmentStorageByte_ = EEPROM_TOURNAMENT_SCORING_BYTE;
         break;

      case MACHINE_STATE_ADJUST_TILT_WARNING:
         adjustmentValues_[1]          = 2;
         currentAdjustmentByte_        = &settings_->maxTiltWarnings;
         currentAdjustmentStorageByte_ = EEPROM_TILT_WARNING_BYTE;
         break;

      case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
         adjustmentType_      = ADJ_TYPE_MIN_MAX_DEFAULT;
         adjustmentValues_[1] = 7;
         currentAdjustmentByte_        = &settings_->scoreAwardReplay;
         currentAdjustmentStorageByte_ = EEPROM_AWARD_OVERRIDE_BYTE;
         break;

      case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
         adjustmentType_      = ADJ_TYPE_LIST;
         numAdjustmentValues_ = 3;
         adjustmentValues_[0] = 3;
         adjustmentValues_[1] = 5;
         adjustmentValues_[2] = 99;
         currentAdjustmentByte_        = &settings_->ballsPerGame;
         currentAdjustmentStorageByte_ = EEPROM_BALLS_OVERRIDE_BYTE;
         break;

      case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
         currentAdjustmentByte_        = (uint8_t*)&settings_->scrollingScores;
         currentAdjustmentStorageByte_ = EEPROM_SCROLLING_SCORES_BYTE;
         break;

      case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
         adjustmentType_               = ADJ_TYPE_SCORE_WITH_DEFAULT;
         currentAdjustmentUL_          = &settings_->extraBallValue;
         currentAdjustmentStorageByte_ = EEPROM_EXTRA_BALL_SCORE_BYTE;
         break;

      case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
         adjustmentType_               = ADJ_TYPE_SCORE_WITH_DEFAULT;
         currentAdjustmentUL_          = &settings_->specialValue;
         currentAdjustmentStorageByte_ = EEPROM_SPECIAL_SCORE_BYTE;
         break;

      case MACHINE_STATE_ADJUST_DIM_LEVEL:
         adjustmentType_      = ADJ_TYPE_LIST;
         numAdjustmentValues_ = 2;
         adjustmentValues_[0] = 2;
         adjustmentValues_[1] = 3;
         currentAdjustmentByte_        = &settings_->dimLevel;
         currentAdjustmentStorageByte_ = EEPROM_DIM_LEVEL_BYTE;
         for (int count = 0; count < 10; count++) {
            machine_->setLampState(BONUS_1 + count, true, 1);
         }
         break;

      case MACHINE_STATE_ADJUST_DONE:
         returnState = MACHINE_STATE_ATTRACT;
         break;
      }
   }

   if (curSwitch == SW_CREDIT_RESET) {
      if (currentAdjustmentByte_ != nullptr &&
          (adjustmentType_ == ADJ_TYPE_MIN_MAX ||
           adjustmentType_ == ADJ_TYPE_MIN_MAX_DEFAULT)) {
         uint8_t curVal = *currentAdjustmentByte_;
         curVal += 1;
         if (curVal > adjustmentValues_[1]) {
            if (adjustmentType_ == ADJ_TYPE_MIN_MAX) {
               curVal = adjustmentValues_[0];
            } else {
               curVal = (curVal > 99) ? adjustmentValues_[0] : 99;
            }
         }
         *currentAdjustmentByte_ = curVal;
         if (currentAdjustmentStorageByte_ != 0) {
            EEPROM.write(currentAdjustmentStorageByte_, curVal);
         }
      } else if (currentAdjustmentByte_ != nullptr &&
                 adjustmentType_ == ADJ_TYPE_LIST) {
         uint8_t curVal   = *currentAdjustmentByte_;
         uint8_t newIndex = 0;
         for (uint8_t valCount = 0; valCount < (numAdjustmentValues_ - 1); valCount++) {
            if (curVal == adjustmentValues_[valCount]) {
               newIndex = valCount + 1;
            }
         }
         *currentAdjustmentByte_ = adjustmentValues_[newIndex];
         if (currentAdjustmentStorageByte_ != 0) {
            EEPROM.write(currentAdjustmentStorageByte_, adjustmentValues_[newIndex]);
         }
      } else if (currentAdjustmentUL_ != nullptr &&
                 (adjustmentType_ == ADJ_TYPE_SCORE_WITH_DEFAULT ||
                  adjustmentType_ == ADJ_TYPE_SCORE_NO_DEFAULT)) {
         unsigned long curVal = *currentAdjustmentUL_;
         curVal += 5000;
         if (curVal > 100000) curVal = 0;
         if (adjustmentType_ == ADJ_TYPE_SCORE_NO_DEFAULT && curVal == 0) curVal = 5000;
         *currentAdjustmentUL_ = curVal;
         if (currentAdjustmentStorageByte_ != 0) {
            machine_->writeULToEEProm(currentAdjustmentStorageByte_, curVal);
         }
      }

      if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
         machine_->setDimDivisor(1, settings_->dimLevel);
      }
   }

   if (currentAdjustmentByte_ != nullptr) {
      machine_->setDisplay(0, (unsigned long)(*currentAdjustmentByte_), true);
   } else if (currentAdjustmentUL_ != nullptr) {
      machine_->setDisplay(0, (*currentAdjustmentUL_), true);
   }

   if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
      for (int count = 0; count < 10; count++) {
         machine_->setLampState(BONUS_1 + count, true, (uint8_t)((currentTime / 1000) % 2));
      }
   }

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }
   return (internalState_ == MACHINE_STATE_ATTRACT) ? TopState::Attract : TopState::Adjustments;
}
