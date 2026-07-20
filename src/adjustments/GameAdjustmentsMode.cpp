/**************************************************************************
 * GameAdjustmentsMode.cpp
 **************************************************************************/

#include "../../include/adjustments/GameAdjustmentsMode.h"
#include "RPU.h"
#include "Trident.h"

void GameAdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = 1;
   stateChanged_  = true;
}

void GameAdjustmentsMode::exit() {
   onExit();
}

TopState GameAdjustmentsMode::update(unsigned long currentTime) {
   onUpdate();

   bool    curStateChanged = stateChanged_;
   stateChanged_           = false;
   uint8_t curState        = internalState_;
   uint8_t returnState     = curState;

   uint8_t curSwitch = machine_->pullFirstFromSwitchStack();

   if (curSwitch == SW_SELF_TEST_SWITCH &&
       (currentTime - selfTestLastPressedTime_) > 250) {
      selfTestLastPressedTime_ = currentTime;
      returnState += 1;
   }

   if (curSwitch == SW_SLAM) {
      returnState = 0;
   }

   if (curState >= 1 && curState <= adjustmentCount()) {
      StoredAdjustment* adj = getAdjustment(curState - 1);

      if (curStateChanged) {
         machine_->stopAllAudio();
         for (int i = 0; i < 4; i++) {
            machine_->setDisplay(i, 0);
            machine_->setDisplayBlank(i, 0x00);
         }
         machine_->setDisplayBallInPlay(curState, true);
         machine_->setDisplayCredits(modeId(), true);
         adj->onEnter(*machine_);
      }

      if (curSwitch == SW_CREDIT_RESET) {
         adj->onPress(*machine_, false);
      }

      adj->onTick(*machine_, currentTime);
   }

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }
   if (internalState_ == 0 || internalState_ > adjustmentCount()) {
      return completedState();
   }
   return activeState();
}
