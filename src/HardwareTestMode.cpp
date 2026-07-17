/**************************************************************************
 * HardwareTestMode.cpp
 **************************************************************************/

#include "HardwareTestMode.h"
#include "Trident2020.h"


void HardwareTestMode::enter(unsigned long currentTime) {
   internalState_ = kTestLamps;
   stateChanged_ = true;
   soundPlaying_ = 0;
   selfTestLastPressedTime_ = currentTime;
}

void HardwareTestMode::exit() {}

TopState HardwareTestMode::update(unsigned long currentTime) {
   bool curStateChanged = stateChanged_;
   stateChanged_ = false;
   uint8_t curState = internalState_;
   uint8_t returnState = curState;

   bool resetDoubleClick = false;
   uint8_t curSwitch = machine_->pullFirstFromSwitchStack();

   if (curSwitch == SW_CREDIT_RESET) {
      if ((currentTime - lastResetPress_) < 400) {
         resetDoubleClick = true;
         curSwitch = 0xFF;
      }
      lastResetPress_ = currentTime;
   }

   if (curSwitch == SW_SLAM) {
      return TopState::Attract;
   }

   if (curSwitch == SW_SELF_TEST_SWITCH && (currentTime - selfTestLastPressedTime_) > 250) {
      returnState += 1;
      selfTestLastPressedTime_ = currentTime;
   }

   if (curStateChanged) {
      machine_->setCoinLockout(false);
      for (int count = 0; count < 4; count++) {
         machine_->setDisplay(count, 0);
         machine_->setDisplayBlank(count, 0x00);
      }
      machine_->setDisplayCredits(curState, true);
      machine_->setDisplayBallInPlay(0, false);
      machine_->stopAllAudio();
   }

   if (curState == kTestLamps) {
      if (curStateChanged) {
         machine_->disableSolenoidStack();
         machine_->setDisableFlippers(true);
         machine_->turnOffAllLamps();
         for (int count = 0; count < machine_->getMaxLamps(); count++) {
            machine_->setLampState(count, true, 0, 500);
         }
         curValue_ = 99;
         machine_->setDisplay(0, curValue_, true);
      }
      if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
         curValue_ += 1;
         if (curValue_ == machine_->getMaxLamps()) {
            curValue_ = 99;
         } else if (curValue_ > 99) {
            curValue_ = 0;
         }
         if (curValue_ == 99) {
            for (int count = 0; count < machine_->getMaxLamps(); count++) {
               machine_->setLampState(count, true, 0, 500);
            }
         } else {
            machine_->turnOffAllLamps();
            machine_->setLampState(curValue_, true);
         }
         machine_->setDisplay(0, curValue_, true);
      }
   } else if (curState == kTestDisplays) {
      if (curStateChanged) {
         machine_->turnOffAllLamps();
         for (int count = 0; count < 4; count++) {
            machine_->setDisplayBlank(count, PinballMachine::ALL_DIGITS_MASK);
         }
         curValue_ = 0;
      }
      if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
         curValue_ += 1;
         if (curValue_ > machine_->getTotalDisplayDigits()) {
            for (int count = 0; count < 4; count++) {
               machine_->setDisplayBlank(count, PinballMachine::ALL_DIGITS_MASK);
            }
            curValue_ = 0;
         }
      }
      machine_->cycleAllDisplays(currentTime, curValue_);
   } else if (curState == kTestSolenoids) {
      if (curStateChanged) {
         machine_->turnOffAllLamps();
         lastSolTestTime_ = currentTime;
         machine_->enableSolenoidStack();
         machine_->setDisableFlippers(false);
         solenoidCycle_ = true;
         solenoidIndex_ = 0;
         machine_->pushToSolenoidStack(solenoidIndex_, 10);
      }
      if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
         solenoidCycle_ = !solenoidCycle_;
      }
      if ((currentTime - lastSolTestTime_) > 1000) {
         if (solenoidCycle_) {
            solenoidIndex_ += 1;
            if (solenoidIndex_ > 14) {
               solenoidIndex_ = 0;
            }
         }
         machine_->pushToSolenoidStack(solenoidIndex_, 10);
         machine_->setDisplay(0, solenoidIndex_, true);
         lastSolTestTime_ = currentTime;
      }
   } else if (curState == kTestSwitches) {
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
   } else if (curState == kTestSounds) {
#if defined(RPU_OS_USE_SB100)
      uint8_t soundToPlay = 0x01 << (((currentTime - selfTestLastPressedTime_) / 750) % 8);
      if (soundPlaying_ != soundToPlay) {
         machine_->playNativeSound(soundToPlay);
         soundPlaying_ = soundToPlay;
         machine_->setDisplay(0, (unsigned long)soundToPlay, true);
         lastSolTestTime_ = currentTime;
      }
#elif defined(RPU_OS_USE_S_AND_T)
      uint8_t soundToPlay = ((currentTime - selfTestLastPressedTime_) / 2000) % 256;
      if (soundPlaying_ != soundToPlay) {
         machine_->playNativeSound(soundToPlay);
         soundPlaying_ = soundToPlay;
         machine_->setDisplay(0, (unsigned long)soundToPlay, true);
         lastSolTestTime_ = currentTime;
      }
#elif defined(RPU_OS_USE_DASH51)
      uint8_t soundToPlay = ((currentTime - selfTestLastPressedTime_) / 2000) % 32;
      if (soundPlaying_ != soundToPlay) {
         if (soundToPlay == 17) {
            soundToPlay = 0;
         }
         machine_->playNativeSound(soundToPlay);
         soundPlaying_ = soundToPlay;
         machine_->setDisplay(0, (unsigned long)soundToPlay, true);
         lastSolTestTime_ = currentTime;
      }
#endif
   }

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_ = true;
   }

   if (internalState_ == 0) {
      return TopState::Attract;
   }
   if (internalState_ > kTestSounds) {
      return TopState::StoredAdjustments;
   }
   return TopState::HardwareTest;
}
