/**************************************************************************
 * HardwareTestMode.cpp
 **************************************************************************/

#include "HardwareTestMode.h"
#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"

#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
#  ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
constexpr uint8_t kTotalDisplayDigits = 34;
#  else
constexpr uint8_t kTotalDisplayDigits = 35;
#  endif
#else
constexpr uint8_t kTotalDisplayDigits = 30;
#endif

// Callout index 0 = test 1 (LAMPS) … index 4 = test 5 (SOUNDS).
static const uint8_t kHardwareTestCalloutMap[5] = { 136, 137, 135, 134, 133 };

void HardwareTestMode::enter(unsigned long /*currentTime*/) {
   internalState_ = kTestLamps;
   stateChanged_  = true;
   soundPlaying_  = 0;
}

void HardwareTestMode::exit() {}

TopState HardwareTestMode::update(unsigned long currentTime) {
   bool    curStateChanged = stateChanged_;
   stateChanged_           = false;
   uint8_t curState        = internalState_;
   uint8_t returnState     = curState;

   bool    resetDoubleClick = false;
   uint8_t curSwitch        = machine_->pullFirstFromSwitchStack();

   if (curSwitch == SW_CREDIT_RESET) {
      if ((currentTime - lastResetPress_) < 400) {
         resetDoubleClick = true;
         curSwitch        = 0xFF;
      }
      lastResetPress_ = currentTime;
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

   if (curStateChanged) {
      machine_->setCoinLockout(false);
      for (int count = 0; count < 4; count++) {
         machine_->setDisplay(count, 0);
         machine_->setDisplayBlank(count, 0x00);
      }
      machine_->setDisplayCredits(curState, true);
      machine_->setDisplayBallInPlay(0, false);
      machine_->stopAllAudio();
      machine_->playCallout(kHardwareTestCalloutMap[curState - 1]);
   }

   if (curState == kTestLamps) {
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
   } else if (curState == kTestDisplays) {
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
            if (solenoidIndex_ > 14) solenoidIndex_ = 0;
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
      uint8_t soundToPlay = 0x01 << (((currentTime - machine_->getSelfTestChangedTime()) / 750) % 8);
      if (soundPlaying_ != soundToPlay) {
         machine_->playSoundCardEffect(soundToPlay);
         soundPlaying_    = soundToPlay;
         machine_->setDisplay(0, (unsigned long)soundToPlay, true);
         lastSolTestTime_ = currentTime;
      }
#elif defined(RPU_OS_USE_S_AND_T)
      uint8_t soundToPlay = ((currentTime - machine_->getSelfTestChangedTime()) / 2000) % 256;
      if (soundPlaying_ != soundToPlay) {
         machine_->playSoundCardEffect(soundToPlay);
         soundPlaying_    = soundToPlay;
         machine_->setDisplay(0, (unsigned long)soundToPlay, true);
         lastSolTestTime_ = currentTime;
      }
#elif defined(RPU_OS_USE_DASH51)
      uint8_t soundToPlay = ((currentTime - machine_->getSelfTestChangedTime()) / 2000) % 32;
      if (soundPlaying_ != soundToPlay) {
         if (soundToPlay == 17) soundToPlay = 0;
         machine_->playSoundCardEffect(soundToPlay);
         soundPlaying_    = soundToPlay;
         machine_->setDisplay(0, (unsigned long)soundToPlay, true);
         lastSolTestTime_ = currentTime;
      }
#endif
   }

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }

   if (internalState_ == 0) return TopState::Attract;
   if (internalState_ > kTestSounds) return TopState::MachineEeprom;
   return TopState::HardwareTest;
}
