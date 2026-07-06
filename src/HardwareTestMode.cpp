/**************************************************************************
 * HardwareTestMode.cpp
 **************************************************************************/

#include "HardwareTestMode.h"
#include "MachineState.h"
#include "RPU.h"
#include "SelfTestAndAudit.h"
#include "SoundEffects.h"
#include "Trident2020.h"

// Callouts for hardware test states -1 (LAMPS) through -20 (TEST_DONE/CPC_CHUTE_3).
static const uint8_t kHardwareTestCalloutMap[20] = {
   136, 137, 135, 134, 133, 140, 141, 142, 139, 143,
   144, 145, 146, 147, 148, 149, 138, 150, 151, 152
};

void HardwareTestMode::enter(unsigned long /*currentTime*/) {
   internalState_ = MACHINE_STATE_TEST_LAMPS;
   stateChanged_  = true;
   soundPlaying_  = 0;
}

void HardwareTestMode::exit() {}

TopState HardwareTestMode::update(unsigned long currentTime) {
   bool curStateChanged = stateChanged_;
   stateChanged_        = false;
   int curState         = internalState_;
   int returnState      = curState;

   bool resetDoubleClick         = false;
   unsigned short savedScoreStartByte  = 0;
   unsigned short auditNumStartByte    = 0;
   unsigned short cpcSelectorStartByte = 0;

   uint8_t curSwitch = machine_->pullFirstFromSwitchStack();

   if (curSwitch == SW_CREDIT_RESET) {
      resetHold_ = currentTime;
      if ((currentTime - lastResetPress_) < 400) {
         resetDoubleClick = true;
         curSwitch        = 0xFF;
      }
      lastResetPress_ = currentTime;
      soundToPlay_ += 1;
      if (soundToPlay_ > 31) soundToPlay_ = 0;
   }

   if (resetHold_ != 0 && !machine_->readSingleSwitchState(SW_CREDIT_RESET)) {
      resetHold_             = 0;
      nextSpeedyValueChange_ = 0;
   }

   bool resetBeingHeld = false;
   if (resetHold_ != 0 && (currentTime - resetHold_) > 1300) {
      resetBeingHeld = true;
      if (nextSpeedyValueChange_ == 0) {
         nextSpeedyValueChange_ = currentTime;
         numSpeedyChanges_      = 0;
      }
   }

   if (curSwitch == SW_SLAM) {
      return TopState::Attract;
   }

   if (curSwitch == SW_SELF_TEST_SWITCH &&
       (currentTime - GetLastSelfTestChangedTime()) > 250) {
      if (machine_->getUpDownSwitchState()) {
         returnState -= 1;
      } else {
         returnState += 1;
      }
      SetLastSelfTestChangedTime(currentTime);
   }

   if (curStateChanged) {
      machine_->setCoinLockout(false);
      for (int count = 0; count < 4; count++) {
         machine_->setDisplay(count, 0);
         machine_->setDisplayBlank(count, 0x00);
      }
      if (curState <= MACHINE_STATE_TEST_SCORE_LEVEL_1) {
         machine_->setDisplayCredits(0, false);
         machine_->setDisplayBallInPlay(MACHINE_STATE_TEST_SOUNDS - curState);
      } else {
         machine_->setDisplayCredits((uint8_t)(0 - curState), true);
         machine_->setDisplayBallInPlay(0, false);
      }
      machine_->stopAllAudio();
      machine_->playCallout(kHardwareTestCalloutMap[-1 - curState]);
   }

   // CPC change detection (states -18, -19, -20)
#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE
   uint8_t chuteNum     = 0xFF;
   uint8_t cpcSelBefore = 0xFF;
   if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_1) {
      chuteNum = 0; cpcSelBefore = machine_->getCPCSelection(0);
   } else if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_2) {
      chuteNum = 1; cpcSelBefore = machine_->getCPCSelection(1);
   } else if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_3) {
      chuteNum = 2; cpcSelBefore = machine_->getCPCSelection(2);
   }
#endif

   // --- Per-state handling ---
   if (curState == MACHINE_STATE_TEST_LAMPS) {
      if (curStateChanged) {
         machine_->disableSolenoidStack();
         machine_->setDisableFlippers(true);
         machine_->turnOffAllLamps();
         for (int count = 0; count < RPU_MAX_LAMPS; count++) {
            machine_->setLampState(count, true, 0, 500);
         }
         curValue_ = 99;
         machine_->setDisplay(0, curValue_, true);
      }
      if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
         if (machine_->getUpDownSwitchState()) {
            curValue_ += 1;
            if (curValue_ == RPU_MAX_LAMPS) {
               curValue_ = 99;
            } else if (curValue_ > 99) {
               curValue_ = 0;
            }
         } else {
            if (curValue_ > 0) {
               curValue_ -= 1;
            } else {
               curValue_ = 99;
            }
            if (curValue_ == 98) curValue_ = RPU_MAX_LAMPS - 1;
         }
         if (curValue_ == 99) {
            for (int count = 0; count < RPU_MAX_LAMPS; count++) {
               machine_->setLampState(count, true, 0, 500);
            }
         } else {
            machine_->turnOffAllLamps();
            machine_->setLampState(curValue_, true);
         }
         machine_->setDisplay(0, curValue_, true);
      }
   } else if (curState == MACHINE_STATE_TEST_DISPLAYS) {
      if (curStateChanged) {
         machine_->turnOffAllLamps();
         for (int count = 0; count < 4; count++) {
            machine_->setDisplayBlank(count, RPU_OS_ALL_DIGITS_MASK);
         }
         curValue_ = 0;
      }
      if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
         if (machine_->getUpDownSwitchState()) {
            curValue_ += 1;
            if (curValue_ > kTotalDisplayDigits) {
               for (int count = 0; count < 4; count++) {
                  machine_->setDisplayBlank(count, RPU_OS_ALL_DIGITS_MASK);
               }
               curValue_ = 0;
            }
         } else {
            if (curValue_ > 0) {
               curValue_ -= 1;
            } else {
               curValue_ = kTotalDisplayDigits;
            }
         }
      }
      machine_->cycleAllDisplays(currentTime, curValue_);
   } else if (curState == MACHINE_STATE_TEST_SOLENOIDS) {
      if (curStateChanged) {
         machine_->turnOffAllLamps();
         lastSolTestTime_ = currentTime;
         machine_->enableSolenoidStack();
         machine_->setDisableFlippers(false);
         solenoidCycle_ = true;
         savedValue_    = 0;
         machine_->pushToSolenoidStack((uint8_t)savedValue_, 10);
      }
      if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
         solenoidCycle_ = !solenoidCycle_;
      }
      if ((currentTime - lastSolTestTime_) > 1000) {
         if (solenoidCycle_) {
            savedValue_ += 1;
            if (savedValue_ > 14) savedValue_ = 0;
         }
         machine_->pushToSolenoidStack((uint8_t)savedValue_, 10);
         machine_->setDisplay(0, savedValue_, true);
         lastSolTestTime_ = currentTime;
      }
   } else if (curState == MACHINE_STATE_TEST_SWITCHES) {
      if (curStateChanged) {
         machine_->turnOffAllLamps();
         machine_->disableSolenoidStack();
         machine_->setDisableFlippers(true);
      }
      uint8_t displayOutput = 0;
      for (uint8_t switchCount = 0; switchCount < 64 && displayOutput < 4; switchCount++) {
         if (machine_->readSingleSwitchState(switchCount)) {
            machine_->setDisplay(displayOutput, switchCount, true);
            displayOutput += 1;
         }
      }
      for (int count = displayOutput; count < 4; count++) {
         machine_->setDisplayBlank(count, 0x00);
      }
   } else if (curState == MACHINE_STATE_TEST_SOUNDS) {
#if defined(RPU_OS_USE_SB100)
      uint8_t soundToPlay = 0x01 << (((currentTime - GetLastSelfTestChangedTime()) / 750) % 8);
      if (soundPlaying_ != soundToPlay) {
         machine_->playSoundCardEffect(soundToPlay);
         soundPlaying_    = soundToPlay;
         machine_->setDisplay(0, (unsigned long)soundToPlay, true);
         lastSolTestTime_ = currentTime;
      }
#elif defined(RPU_OS_USE_S_AND_T)
      uint8_t soundToPlay = ((currentTime - GetLastSelfTestChangedTime()) / 2000) % 256;
      if (soundPlaying_ != soundToPlay) {
         machine_->playSoundCardEffect(soundToPlay);
         soundPlaying_    = soundToPlay;
         machine_->setDisplay(0, (unsigned long)soundToPlay, true);
         lastSolTestTime_ = currentTime;
      }
#elif defined(RPU_OS_USE_DASH51)
      uint8_t soundToPlay = ((currentTime - GetLastSelfTestChangedTime()) / 2000) % 32;
      if (soundPlaying_ != soundToPlay) {
         if (soundToPlay == 17) soundToPlay = 0;
         machine_->playSoundCardEffect(soundToPlay);
         soundPlaying_    = soundToPlay;
         machine_->setDisplay(0, (unsigned long)soundToPlay, true);
         lastSolTestTime_ = currentTime;
      }
#endif
   } else if (curState == MACHINE_STATE_TEST_BOOT) {
      if (curStateChanged) {
         for (int count = 0; count < 4; count++) {
            machine_->setDisplay(count, 8007, true);
         }
      }
      if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
         returnState = MACHINE_STATE_ATTRACT;
      }
      for (int count = 0; count < 4; count++) {
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
         machine_->setDisplayBlank(count, ((currentTime / 500) % 2) ? 0x78 : 0x00);
#else
         machine_->setDisplayBlank(count, ((currentTime / 500) % 2) ? 0x3C : 0x00);
#endif
      }
   } else if (curState == MACHINE_STATE_TEST_SCORE_LEVEL_1) {
#ifdef RPU_OS_USE_SB100
      if (curStateChanged) machine_->playSoundCardEffect(0);
#endif
      savedScoreStartByte = RPU_AWARD_SCORE_1_EEPROM_START_BYTE;
   } else if (curState == MACHINE_STATE_TEST_SCORE_LEVEL_2) {
      savedScoreStartByte = RPU_AWARD_SCORE_2_EEPROM_START_BYTE;
   } else if (curState == MACHINE_STATE_TEST_SCORE_LEVEL_3) {
      savedScoreStartByte = RPU_AWARD_SCORE_3_EEPROM_START_BYTE;
   } else if (curState == MACHINE_STATE_TEST_HISCR) {
      savedScoreStartByte = RPU_HIGHSCORE_EEPROM_START_BYTE;
   } else if (curState == MACHINE_STATE_TEST_CREDITS) {
      if (curStateChanged) {
         savedValue_ = machine_->readByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
         machine_->setDisplay(0, savedValue_, true);
      }
      if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
         if (machine_->getUpDownSwitchState()) {
            savedValue_ += 1;
            if (savedValue_ > 99) savedValue_ = 0;
         } else {
            if (savedValue_ > 0) savedValue_ -= 1;
            else savedValue_ = 99;
         }
         machine_->setDisplay(0, savedValue_, true);
         machine_->writeByteToEEProm(RPU_CREDITS_EEPROM_BYTE, (uint8_t)(savedValue_ & 0xFF));
      }
   } else if (curState == MACHINE_STATE_TEST_TOTAL_PLAYS) {
      auditNumStartByte = RPU_TOTAL_PLAYS_EEPROM_START_BYTE;
   } else if (curState == MACHINE_STATE_TEST_TOTAL_REPLAYS) {
      auditNumStartByte = RPU_TOTAL_REPLAYS_EEPROM_START_BYTE;
   } else if (curState == MACHINE_STATE_TEST_HISCR_BEAT) {
      auditNumStartByte = RPU_TOTAL_HISCORE_BEATEN_START_BYTE;
   } else if (curState == MACHINE_STATE_TEST_CHUTE_2_COINS) {
      auditNumStartByte = RPU_CHUTE_2_COINS_START_BYTE;
   } else if (curState == MACHINE_STATE_TEST_CHUTE_1_COINS) {
      auditNumStartByte = RPU_CHUTE_1_COINS_START_BYTE;
   } else if (curState == MACHINE_STATE_TEST_CHUTE_3_COINS) {
      auditNumStartByte = RPU_CHUTE_3_COINS_START_BYTE;
#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE
   } else if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_1) {
      cpcSelectorStartByte = RPU_CPC_CHUTE_1_SELECTION_BYTE;
   } else if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_2) {
      cpcSelectorStartByte = RPU_CPC_CHUTE_2_SELECTION_BYTE;
   } else if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_3) {
      cpcSelectorStartByte = RPU_CPC_CHUTE_3_SELECTION_BYTE;
#endif
   }

   if (savedScoreStartByte) {
      if (curStateChanged) {
         savedValue_ = machine_->readULFromEEProm(savedScoreStartByte);
         machine_->setDisplay(0, savedValue_, true);
      }
      if (curSwitch == SW_CREDIT_RESET) {
         if (machine_->getUpDownSwitchState()) {
            savedValue_ += 1000;
         } else {
            if (savedValue_ > 1000) savedValue_ -= 1000;
            else savedValue_ = 0;
         }
         machine_->setDisplay(0, savedValue_, true);
         machine_->writeULToEEProm(savedScoreStartByte, savedValue_);
      }
      if (resetBeingHeld && (currentTime >= nextSpeedyValueChange_)) {
         if (machine_->getUpDownSwitchState()) {
            savedValue_ += 1000;
         } else {
            if (savedValue_ > 1000) savedValue_ -= 1000;
            else savedValue_ = 0;
         }
         machine_->setDisplay(0, savedValue_, true);
         if (numSpeedyChanges_ < 6) {
            nextSpeedyValueChange_ = currentTime + 400;
         } else if (numSpeedyChanges_ < 50) {
            nextSpeedyValueChange_ = currentTime + 50;
         } else {
            nextSpeedyValueChange_ = currentTime + 10;
         }
         numSpeedyChanges_ += 1;
      }
      if (!resetBeingHeld && numSpeedyChanges_ > 0) {
         machine_->writeULToEEProm(savedScoreStartByte, savedValue_);
         numSpeedyChanges_ = 0;
      }
      if (resetDoubleClick) {
         savedValue_ = 0;
         machine_->setDisplay(0, savedValue_, true);
         machine_->writeULToEEProm(savedScoreStartByte, savedValue_);
      }
   }

   if (auditNumStartByte) {
      if (curStateChanged) {
         savedValue_ = machine_->readULFromEEProm(auditNumStartByte);
         machine_->setDisplay(0, savedValue_, true);
      }
      if (resetDoubleClick) {
         savedValue_ = 0;
         machine_->setDisplay(0, savedValue_, true);
         machine_->writeULToEEProm(auditNumStartByte, savedValue_);
      }
   }

#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE
   if (cpcSelectorStartByte) {
      if (curStateChanged) {
         savedValue_ = machine_->readByteFromEEProm(cpcSelectorStartByte);
         if (savedValue_ >= kNumCPCPairs) savedValue_ = 4;
         machine_->setDisplay(0, machine_->getCPCPairCoins((uint8_t)savedValue_), true);
         machine_->setDisplay(1, machine_->getCPCPairCredits((uint8_t)savedValue_), true);
      }
      if (curSwitch == SW_CREDIT_RESET) {
         uint8_t lastValue = (uint8_t)savedValue_;
         if (machine_->getUpDownSwitchState()) {
            savedValue_ += 1;
            if (savedValue_ >= kNumCPCPairs) savedValue_ = 0;
         } else {
            if (savedValue_ > 0) savedValue_ -= 1;
         }
         machine_->setDisplay(0, machine_->getCPCPairCoins((uint8_t)savedValue_), true);
         machine_->setDisplay(1, machine_->getCPCPairCredits((uint8_t)savedValue_), true);
         if (lastValue != (uint8_t)savedValue_) {
            if (cpcSelectorStartByte == RPU_CPC_CHUTE_1_SELECTION_BYTE) {
               machine_->setCPCSelection(0, (uint8_t)savedValue_);
            } else if (cpcSelectorStartByte == RPU_CPC_CHUTE_2_SELECTION_BYTE) {
               machine_->setCPCSelection(1, (uint8_t)savedValue_);
            } else if (cpcSelectorStartByte == RPU_CPC_CHUTE_3_SELECTION_BYTE) {
               machine_->setCPCSelection(2, (uint8_t)savedValue_);
            }
         }
      }
   }

   // Notify if CPC selection changed this tick.
   if (chuteNum != 0xFF && cpcSelBefore != machine_->getCPCSelection(chuteNum)) {
      machine_->stopAllAudio();
      machine_->playCallout(SOUND_EFFECT_SELF_TEST_CPC_START + machine_->getCPCSelection(chuteNum));
   }
#endif

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }

   if (internalState_ == MACHINE_STATE_ATTRACT) return TopState::Attract;
   if (internalState_ < MACHINE_STATE_TEST_DONE) return TopState::Adjustments;
   return TopState::HardwareTest;
}
