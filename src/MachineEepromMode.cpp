/**************************************************************************
 * MachineEepromMode.cpp
 **************************************************************************/

#include "MachineEepromMode.h"
#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"

// Callout index 0 = setting 1 (score level 1) … index 11 = setting 12 (boot).
static const uint8_t kMachineEepromCalloutMap[12] = {
   140, 141, 142, 139, 143, 144, 145, 146, 147, 148, 149, 138
};

void MachineEepromMode::enter(unsigned long /*currentTime*/) {
   internalState_ = kEepromScoreLevel1;
   stateChanged_  = true;
}

void MachineEepromMode::exit() {}

TopState MachineEepromMode::update(unsigned long currentTime) {
   bool    curStateChanged = stateChanged_;
   stateChanged_           = false;
   uint8_t curState        = internalState_;
   uint8_t returnState     = curState;

   bool    resetDoubleClick = false;
   uint8_t curSwitch        = machine_->pullFirstFromSwitchStack();

   if (curSwitch == SW_CREDIT_RESET) {
      resetHold_ = currentTime;
      if ((currentTime - lastResetPress_) < 400) {
         resetDoubleClick = true;
         curSwitch        = 0xFF;
      }
      lastResetPress_ = currentTime;
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
       (currentTime - machine_->getSelfTestChangedTime()) > 250) {
      if (machine_->getUpDownSwitchState()) {
         returnState += 1;
      } else {
         returnState -= 1;
      }
      machine_->setSelfTestChangedTime(currentTime);
   }

   // Clamp backward at start; 0 is reserved for "exit to Attract" (e.g. from BOOT).
   if (returnState == 0 && curState != 0) {
      returnState = kEepromScoreLevel1;
   }

   if (curStateChanged) {
      machine_->setCoinLockout(false);
      for (int count = 0; count < 4; count++) {
         machine_->setDisplay(count, 0);
         machine_->setDisplayBlank(count, 0x00);
      }
      machine_->setDisplayCredits(0, false);
      machine_->setDisplayBallInPlay(curState);
      machine_->stopAllAudio();
      machine_->playCallout(kMachineEepromCalloutMap[curState - 1]);
   }

   unsigned short savedScoreStartByte  = 0;
   unsigned short auditNumStartByte    = 0;

   if (curState == kEepromScoreLevel1) {
#ifdef RPU_OS_USE_SB100
      if (curStateChanged) machine_->playSoundCardEffect(0);
#endif
      savedScoreStartByte = RPU_AWARD_SCORE_1_EEPROM_START_BYTE;
   } else if (curState == kEepromScoreLevel2) {
      savedScoreStartByte = RPU_AWARD_SCORE_2_EEPROM_START_BYTE;
   } else if (curState == kEepromScoreLevel3) {
      savedScoreStartByte = RPU_AWARD_SCORE_3_EEPROM_START_BYTE;
   } else if (curState == kEepromHighScore) {
      savedScoreStartByte = RPU_HIGHSCORE_EEPROM_START_BYTE;
   } else if (curState == kEepromCredits) {
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
   } else if (curState == kEepromTotalPlays) {
      auditNumStartByte = RPU_TOTAL_PLAYS_EEPROM_START_BYTE;
   } else if (curState == kEepromTotalReplays) {
      auditNumStartByte = RPU_TOTAL_REPLAYS_EEPROM_START_BYTE;
   } else if (curState == kEepromHiscrBeat) {
      auditNumStartByte = RPU_TOTAL_HISCORE_BEATEN_START_BYTE;
   } else if (curState == kEepromChute2Coins) {
      auditNumStartByte = RPU_CHUTE_2_COINS_START_BYTE;
   } else if (curState == kEepromChute1Coins) {
      auditNumStartByte = RPU_CHUTE_1_COINS_START_BYTE;
   } else if (curState == kEepromChute3Coins) {
      auditNumStartByte = RPU_CHUTE_3_COINS_START_BYTE;
   } else if (curState == kEepromBoot) {
      if (curStateChanged) {
         for (int count = 0; count < 4; count++) {
            machine_->setDisplay(count, 8007, true);
         }
      }
      if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
         returnState = 0;   // exit to Attract
      }
      for (int count = 0; count < 4; count++) {
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
         machine_->setDisplayBlank(count, ((currentTime / 500) % 2) ? 0x78 : 0x00);
#else
         machine_->setDisplayBlank(count, ((currentTime / 500) % 2) ? 0x3C : 0x00);
#endif
      }
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

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }

   if (internalState_ == 0) return TopState::Attract;
   if (internalState_ > kEepromBoot) return TopState::Adjustments;
   return TopState::MachineEeprom;
}
