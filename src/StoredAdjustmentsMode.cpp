/**************************************************************************
 * StoredAdjustmentsMode.cpp
 **************************************************************************/

#include "StoredAdjustmentsMode.h"
#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include <EEPROM.h>

constexpr uint8_t kNumCPCPairs = 9;

// Callout index 0 = setting 1 (score level 1) … index 14 = setting 15 (CPC chute 3).
static const uint8_t kStoredAdjCalloutMap[15] = {
   140, 141, 142, 139, 143, 144, 145, 146, 147, 148, 149, 138, 150, 151, 152
};

static unsigned long readUL(int addr) {
   unsigned long val = 0;
   EEPROM.get(addr, val);
   return val;
}

static void writeUL(int addr, unsigned long val) {
   EEPROM.put(addr, val);
}

void StoredAdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = kSaScoreLevel1;
   stateChanged_  = true;
}

void StoredAdjustmentsMode::exit() {}

TopState StoredAdjustmentsMode::update(unsigned long currentTime) {
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
      returnState = kSaScoreLevel1;
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
      machine_->playCallout(kStoredAdjCalloutMap[curState - 1]);
   }

   // CPC change detection.
   uint8_t chuteNum     = 0xFF;
   uint8_t cpcSelBefore = 0xFF;
   if (curState == kSaCpcChute1) {
      chuteNum = 0; cpcSelBefore = machine_->getCPCSelection(0);
   } else if (curState == kSaCpcChute2) {
      chuteNum = 1; cpcSelBefore = machine_->getCPCSelection(1);
   } else if (curState == kSaCpcChute3) {
      chuteNum = 2; cpcSelBefore = machine_->getCPCSelection(2);
   }

   int           savedScoreAddr = 0;
   int           auditAddr      = 0;
   unsigned short cpcSelectorStartByte = 0;

   if (curState == kSaScoreLevel1) {
      savedScoreAddr = kEeScoreLevel1;
   } else if (curState == kSaScoreLevel2) {
      savedScoreAddr = kEeScoreLevel2;
   } else if (curState == kSaScoreLevel3) {
      savedScoreAddr = kEeScoreLevel3;
   } else if (curState == kSaHighScore) {
      savedScoreAddr = kEeHighScore;
   } else if (curState == kSaCredits) {
      if (curStateChanged) {
         savedValue_ = EEPROM.read(kEeCredits);
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
         EEPROM.write(kEeCredits, (uint8_t)(savedValue_ & 0xFF));
      }
   } else if (curState == kSaTotalPlays) {
      auditAddr = kEeTotalPlays;
   } else if (curState == kSaTotalReplays) {
      auditAddr = kEeTotalReplays;
   } else if (curState == kSaHiscrBeat) {
      auditAddr = kEeHiscrBeat;
   } else if (curState == kSaChute2Coins) {
      auditAddr = kEeChute2Coins;
   } else if (curState == kSaChute1Coins) {
      auditAddr = kEeChute1Coins;
   } else if (curState == kSaChute3Coins) {
      auditAddr = kEeChute3Coins;
   } else if (curState == kSaBoot) {
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
   } else if (curState == kSaCpcChute1) {
      cpcSelectorStartByte = RPU_CPC_CHUTE_1_SELECTION_BYTE;
   } else if (curState == kSaCpcChute2) {
      cpcSelectorStartByte = RPU_CPC_CHUTE_2_SELECTION_BYTE;
   } else if (curState == kSaCpcChute3) {
      cpcSelectorStartByte = RPU_CPC_CHUTE_3_SELECTION_BYTE;
   }

   if (savedScoreAddr) {
      if (curStateChanged) {
         savedValue_ = readUL(savedScoreAddr);
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
         writeUL(savedScoreAddr, savedValue_);
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
         writeUL(savedScoreAddr, savedValue_);
         numSpeedyChanges_ = 0;
      }
      if (resetDoubleClick) {
         savedValue_ = 0;
         machine_->setDisplay(0, savedValue_, true);
         writeUL(savedScoreAddr, savedValue_);
      }
   }

   if (auditAddr) {
      if (curStateChanged) {
         savedValue_ = readUL(auditAddr);
         machine_->setDisplay(0, savedValue_, true);
      }
      if (resetDoubleClick) {
         savedValue_ = 0;
         machine_->setDisplay(0, savedValue_, true);
         writeUL(auditAddr, savedValue_);
      }
   }

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

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }

   if (internalState_ == 0) return TopState::Attract;
   if (internalState_ > kSaCpcChute3) return TopState::Adjustments;
   return TopState::StoredAdjustments;
}
