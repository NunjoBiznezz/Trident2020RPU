/**************************************************************************
 * Trident2020AdjustmentsMode.cpp
 **************************************************************************/

#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include "Trident2020AdjustmentsMode.h"
#include <EEPROM.h>

// Callout index 0 = adjustment 1 (free play) … index 14 = adjustment 15 (done).
static const uint8_t kAdjustmentCalloutMap[15] = {
   153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 171, 0
};

static const uint8_t kSoundSelectorToCalloutsMap[] = {190, 191, 199, 197, 198, 196};

void Trident2020AdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = kAdjFreeplay;
   stateChanged_  = true;
}

void Trident2020AdjustmentsMode::exit() {
   machine_->readStoredParameters();
}

TopState Trident2020AdjustmentsMode::update(unsigned long currentTime) {
   bool    curStateChanged = stateChanged_;
   stateChanged_           = false;
   uint8_t curState        = internalState_;
   uint8_t returnState     = curState;

   game_->setNumPlayers(0);   // ???

   if (curStateChanged) {
      machine_->stopAllAudio();
      uint8_t callout = kAdjustmentCalloutMap[curState - 1];
      machine_->playCallout(callout);
      soundSettingTimeout_ = 0;
   } else {
      if (soundSettingTimeout_ != 0 && currentTime > soundSettingTimeout_) {
         soundSettingTimeout_ = 0;
         machine_->stopAllAudio();
      }
   }

   uint8_t curSwitch = machine_->pullFirstFromSwitchStack();

   // This isn't working well, needs better de-bounce
   if (curSwitch == SW_SELF_TEST_SWITCH &&
       (currentTime - machine_->getSelfTestChangedTime()) > 250) {
      machine_->setSelfTestChangedTime(currentTime);
      returnState += 1;
   }

   if (curSwitch == SW_SLAM) {
      returnState = 0;
   }

   if (curStateChanged) {
      for (int count = 0; count < 4; count++) {
         machine_->setDisplay(count, 0);
         machine_->setDisplayBlank(count, 0x00);
      }
      machine_->setDisplayCredits(curState, true);
      machine_->setDisplayBallInPlay(0, false);
      currentAdjustmentByte_        = nullptr;
      currentAdjustmentUL_          = nullptr;
      currentAdjustmentStorageByte_ = 0;

      adjustmentType_      = ADJ_TYPE_MIN_MAX;
      adjustmentValues_[0] = 0;
      adjustmentValues_[1] = 1;
      tempValue_           = 0;

      switch (curState) {
      case kAdjFreeplay:
         currentAdjustmentByte_        = (uint8_t*)&settings_->freePlayMode;
         currentAdjustmentStorageByte_ = EEPROM_FREE_PLAY_BYTE;
         break;

      case kAdjBallSave:
         adjustmentType_      = ADJ_TYPE_LIST;
         numAdjustmentValues_ = 5;
         adjustmentValues_[1] = 5;
         adjustmentValues_[2] = 10;
         adjustmentValues_[3] = 15;
         adjustmentValues_[4] = 20;
         currentAdjustmentByte_        = &settings_->ballSaveNumSeconds;
         currentAdjustmentStorageByte_ = EEPROM_BALL_SAVE_BYTE;
         break;

      case kAdjSfxAndSoundtrack:
         adjustmentType_      = ADJ_TYPE_MIN_MAX;
         adjustmentValues_[1] = 5;
         currentAdjustmentByte_        = &settings_->soundSelector;
         currentAdjustmentStorageByte_ = EEPROM_SOUND_SELECTOR_BYTE;
         break;

      case kAdjMusicVolume:
         adjustmentType_      = ADJ_TYPE_MIN_MAX;
         adjustmentValues_[1] = 10;
         currentAdjustmentByte_        = &settings_->musicVolume;
         currentAdjustmentStorageByte_ = EEPROM_MUSIC_VOLUME_BYTE;
         break;

      case kAdjSfxVolume:
         adjustmentType_      = ADJ_TYPE_MIN_MAX;
         adjustmentValues_[1] = 10;
         currentAdjustmentByte_        = &settings_->sfxVolume;
         currentAdjustmentStorageByte_ = EEPROM_SFX_VOLUME_BYTE;
         break;

      case kAdjCalloutsVolume:
         adjustmentType_      = ADJ_TYPE_MIN_MAX;
         adjustmentValues_[1] = 10;
         currentAdjustmentByte_        = &settings_->calloutsVolume;
         currentAdjustmentStorageByte_ = EEPROM_CALLOUTS_VOLUME_BYTE;
         break;

      case kAdjTournamentScoring:
         currentAdjustmentByte_        = (uint8_t*)&settings_->tournamentScoring;
         currentAdjustmentStorageByte_ = EEPROM_TOURNAMENT_SCORING_BYTE;
         break;

      case kAdjTiltWarning:
         adjustmentValues_[1]          = 2;
         currentAdjustmentByte_        = &settings_->maxTiltWarnings;
         currentAdjustmentStorageByte_ = EEPROM_TILT_WARNING_BYTE;
         break;

      case kAdjAwardOverride:
         adjustmentType_      = ADJ_TYPE_MIN_MAX_DEFAULT;
         adjustmentValues_[1] = 7;
         currentAdjustmentByte_        = &settings_->trident2020Awards.scoreAwardReplay;
         currentAdjustmentStorageByte_ = EEPROM_AWARD_OVERRIDE_BYTE;
         break;

      case kAdjBallsOverride:
         adjustmentType_      = ADJ_TYPE_LIST;
         numAdjustmentValues_ = 3;
         adjustmentValues_[0] = 3;
         adjustmentValues_[1] = 5;
         adjustmentValues_[2] = 99;
         currentAdjustmentByte_        = &settings_->ballsPerGame;
         currentAdjustmentStorageByte_ = EEPROM_BALLS_OVERRIDE_BYTE;
         break;

      case kAdjScrollingScores:
         currentAdjustmentByte_        = (uint8_t*)&settings_->scrollingScores;
         currentAdjustmentStorageByte_ = EEPROM_SCROLLING_SCORES_BYTE;
         break;

      case kAdjExtraBallAward:
         adjustmentType_               = ADJ_TYPE_SCORE_WITH_DEFAULT;
         currentAdjustmentUL_          = &settings_->trident2020Awards.extraBallValue;
         currentAdjustmentStorageByte_ = EEPROM_EXTRA_BALL_SCORE_BYTE;
         break;

      case kAdjSpecialAward:
         adjustmentType_               = ADJ_TYPE_SCORE_WITH_DEFAULT;
         currentAdjustmentUL_          = &settings_->trident2020Awards.specialValue;
         currentAdjustmentStorageByte_ = EEPROM_SPECIAL_SCORE_BYTE;
         break;

      case kAdjDimLevel:
         adjustmentType_      = ADJ_TYPE_LIST;
         numAdjustmentValues_ = 2;
         adjustmentValues_[0] = 2;
         adjustmentValues_[1] = 3;
         currentAdjustmentByte_        = &settings_->dimLevel;
         currentAdjustmentStorageByte_ = EEPROM_DIM_LEVEL_BYTE;
         for (int count = 0; count < 10; count++) {
            machine_->setLampState(LAMP_BONUS_1 + count, true, 1);
         }
         break;

      case kAdjDone:
         returnState = 0;
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
            EEPROM.put(currentAdjustmentStorageByte_, curVal);
         }
      }

      if (curState == kAdjDimLevel) {
         machine_->setDimDivisor(1, settings_->dimLevel);
      }
   }

   if (currentAdjustmentByte_ != nullptr) {
      machine_->setDisplay(0, (unsigned long)(*currentAdjustmentByte_), true);
   } else if (currentAdjustmentUL_ != nullptr) {
      machine_->setDisplay(0, (*currentAdjustmentUL_), true);
   }

   if (curState == kAdjDimLevel) {
      for (int count = 0; count < 10; count++) {
         machine_->setLampState(LAMP_BONUS_1 + count, true, (uint8_t)((currentTime / 1000) % 2));
      }
   }

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }
   return (internalState_ == 0) ? TopState::Attract : TopState::Adjustments;
}

void Trident2020AdjustmentsMode::enterSubstate(uint8_t subState, unsigned long currentTime) {



}