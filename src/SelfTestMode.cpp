/**************************************************************************
 * SelfTestMode.cpp
 **************************************************************************/

#include "SelfTestMode.h"
#include "RPU.h"
#include "SelfTestAndAudit.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include <EEPROM.h>

static const uint8_t kSelfTestStateToCalloutMap[] = {
   136, 137, 135, 134, 133, 140, 141, 142, 139, 143, 144, 145, 146, 147,
   148, 149, 138, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
   161, 162, 163, 164, 165, 171, 0
};

static const uint8_t kSoundSelectorToCalloutsMap[] = {190, 191, 199, 197, 198, 196};

void SelfTestMode::enter(unsigned long) {
   internalState_ = MACHINE_STATE_TEST_LAMPS;
   stateChanged_  = true;
}

void SelfTestMode::exit() {
   machine_->readStoredParameters();
   if (readParamsCallback_) readParamsCallback_();
}

TopState SelfTestMode::update(unsigned long currentTime) {
   bool curStateChanged = stateChanged_;
   stateChanged_ = false;
   int curState = internalState_;
   int returnState = curState;

   game_->setNumPlayers(0);

   if (curStateChanged) {
      machine_->stopAllAudio();
      unsigned short modeMapping = kSelfTestStateToCalloutMap[-1 - curState];
      machine_->playCallout(modeMapping);
      soundSettingTimeout_ = 0;
   } else {
      if (soundSettingTimeout_ != 0 && currentTime > soundSettingTimeout_) {
         soundSettingTimeout_ = 0;
         machine_->stopAllAudio();
      }
   }

   if (curState >= MACHINE_STATE_TEST_DONE) {
      uint8_t cpcSelection = 0xFF;
      uint8_t chuteNum     = 0xFF;
      if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_1) chuteNum = 0;
      if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_2) chuteNum = 1;
      if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_3) chuteNum = 2;
      if (chuteNum != 0xFF) cpcSelection = GetCPCSelection(chuteNum);

      returnState = RunBaseSelfTest(returnState, curStateChanged, currentTime,
                                    SW_CREDIT_RESET, SW_SLAM);

      if (chuteNum != 0xFF) {
         if (cpcSelection != GetCPCSelection(chuteNum)) {
            uint8_t newCPC = GetCPCSelection(chuteNum);
            machine_->stopAllAudio();
            machine_->playCallout(SOUND_EFFECT_SELF_TEST_CPC_START + newCPC);
         }
      }
   } else {
      uint8_t curSwitch = RPU_PullFirstFromSwitchStack();

      if (curSwitch == SW_SELF_TEST_SWITCH &&
          (currentTime - GetLastSelfTestChangedTime()) > 250) {
         SetLastSelfTestChangedTime(currentTime);
         returnState -= 1;
      }

      if (curSwitch == SW_SLAM) {
         returnState = MACHINE_STATE_ATTRACT;
      }

      if (curStateChanged) {
         for (int count = 0; count < 4; count++) {
            RPU_SetDisplay(count, 0);
            RPU_SetDisplayBlank(count, 0x00);
         }
         RPU_SetDisplayCredits(MACHINE_STATE_TEST_SOUNDS - curState);
         RPU_SetDisplayBallInPlay(0, false);
         currentAdjustmentByte_       = nullptr;
         currentAdjustmentUL_         = nullptr;
         currentAdjustmentStorageByte_ = 0;

         adjustmentType_       = ADJ_TYPE_MIN_MAX;
         adjustmentValues_[0]  = 0;
         adjustmentValues_[1]  = 1;
         tempValue_            = 0;

         switch (curState) {
         case MACHINE_STATE_ADJUST_FREEPLAY:
            currentAdjustmentByte_       = (uint8_t*)settings_.freePlayMode;
            currentAdjustmentStorageByte_ = EEPROM_FREE_PLAY_BYTE;
            break;

         case MACHINE_STATE_ADJUST_BALL_SAVE:
            adjustmentType_      = ADJ_TYPE_LIST;
            numAdjustmentValues_ = 5;
            adjustmentValues_[1] = 5;
            adjustmentValues_[2] = 10;
            adjustmentValues_[3] = 15;
            adjustmentValues_[4] = 20;
            currentAdjustmentByte_       = settings_.ballSaveNumSeconds;
            currentAdjustmentStorageByte_ = EEPROM_BALL_SAVE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK:
            adjustmentType_      = ADJ_TYPE_MIN_MAX;
            adjustmentValues_[1] = 5;
            currentAdjustmentByte_       = machine_->soundSelectorPtr();
            currentAdjustmentStorageByte_ = EEPROM_SOUND_SELECTOR_BYTE;
            break;

         case MACHINE_STATE_ADJUST_MUSIC_VOLUME:
            adjustmentType_      = ADJ_TYPE_MIN_MAX;
            adjustmentValues_[1] = 10;
            currentAdjustmentByte_       = machine_->musicVolumePtr();
            currentAdjustmentStorageByte_ = EEPROM_MUSIC_VOLUME_BYTE;
            break;

         case MACHINE_STATE_ADJUST_SFX_VOLUME:
            adjustmentType_      = ADJ_TYPE_MIN_MAX;
            adjustmentValues_[1] = 10;
            currentAdjustmentByte_       = machine_->sfxVolumePtr();
            currentAdjustmentStorageByte_ = EEPROM_SFX_VOLUME_BYTE;
            break;

         case MACHINE_STATE_ADJUST_CALLOUTS_VOLUME:
            adjustmentType_      = ADJ_TYPE_MIN_MAX;
            adjustmentValues_[1] = 10;
            currentAdjustmentByte_       = machine_->calloutsVolumePtr();
            currentAdjustmentStorageByte_ = EEPROM_CALLOUTS_VOLUME_BYTE;
            break;

         case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
            currentAdjustmentByte_       = (uint8_t*)settings_.tournamentScoring;
            currentAdjustmentStorageByte_ = EEPROM_TOURNAMENT_SCORING_BYTE;
            break;

         case MACHINE_STATE_ADJUST_TILT_WARNING:
            adjustmentValues_[1] = 2;
            currentAdjustmentByte_       = settings_.maxTiltWarnings;
            currentAdjustmentStorageByte_ = EEPROM_TILT_WARNING_BYTE;
            break;

         case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
            adjustmentType_      = ADJ_TYPE_MIN_MAX_DEFAULT;
            adjustmentValues_[1] = 7;
            currentAdjustmentByte_       = settings_.scoreAwardReplay;
            currentAdjustmentStorageByte_ = EEPROM_AWARD_OVERRIDE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
            adjustmentType_      = ADJ_TYPE_LIST;
            numAdjustmentValues_ = 3;
            adjustmentValues_[0] = 3;
            adjustmentValues_[1] = 5;
            adjustmentValues_[2] = 99;
            currentAdjustmentByte_       = settings_.ballsPerGame;
            currentAdjustmentStorageByte_ = EEPROM_BALLS_OVERRIDE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
            currentAdjustmentByte_       = (uint8_t*)settings_.scrollingScores;
            currentAdjustmentStorageByte_ = EEPROM_SCROLLING_SCORES_BYTE;
            break;

         case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
            adjustmentType_             = ADJ_TYPE_SCORE_WITH_DEFAULT;
            currentAdjustmentUL_        = settings_.extraBallValue;
            currentAdjustmentStorageByte_ = EEPROM_EXTRA_BALL_SCORE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
            adjustmentType_             = ADJ_TYPE_SCORE_WITH_DEFAULT;
            currentAdjustmentUL_        = settings_.specialValue;
            currentAdjustmentStorageByte_ = EEPROM_SPECIAL_SCORE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_DIM_LEVEL:
            adjustmentType_      = ADJ_TYPE_LIST;
            numAdjustmentValues_ = 2;
            adjustmentValues_[0] = 2;
            adjustmentValues_[1] = 3;
            currentAdjustmentByte_       = settings_.dimLevel;
            currentAdjustmentStorageByte_ = EEPROM_DIM_LEVEL_BYTE;
            for (int count = 0; count < 10; count++) {
               RPU_SetLampState(BONUS_1 + count, true, 1);
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
               RPU_WriteULToEEProm(currentAdjustmentStorageByte_, curVal);
            }
         }

         if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
            RPU_SetDimDivisor(1, *settings_.dimLevel);
         }
      }

      if (currentAdjustmentByte_ != nullptr) {
         RPU_SetDisplay(0, (unsigned long)(*currentAdjustmentByte_), true);
      } else if (currentAdjustmentUL_ != nullptr) {
         RPU_SetDisplay(0, (*currentAdjustmentUL_), true);
      }
   }

   if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
      for (int count = 0; count < 10; count++) {
         RPU_SetLampState(BONUS_1 + count, true, (currentTime / 1000) % 2);
      }
   }

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }
   return (internalState_ == MACHINE_STATE_ATTRACT) ? TopState::Attract : TopState::SelfTest;
}
