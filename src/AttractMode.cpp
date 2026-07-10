/**************************************************************************
 * AttractMode.cpp
 **************************************************************************/

#include "AttractMode.h"
#include "LampAnimation.h"
#include "MachineState.h"
#include "Trident.h"

#if defined(DEBUG_MESSAGES) && defined(DEBUG_PORT)
#  define DEBUG_MESSAGE(x) DEBUG_PORT.write(x)
#else
#  define DEBUG_MESSAGE(x)
#endif

static constexpr int REPEAT_LAMP_ANIMATIONS = 3;


void AttractMode::enter(unsigned long currentTime) {
   machine_->playSoundCardEffect(0);
   machine_->disableSolenoidStack();
   machine_->turnOffAllLamps();
   machine_->setDisableFlippers(true);
   DEBUG_MESSAGE("Entering Attract Mode\n\r");
   lastPlayfieldMode_ = 0;
   enterTime_         = currentTime;

   savedNumPlayers_ = machine_->getLastGameNumPlayers();
   for (uint8_t i = 0; i < 4; i++) savedScores_[i] = machine_->getLastGameScore(i);

   if (versionMajor_ != 0) {
      machine_->setDisplay(0, versionMajor_);
      machine_->setDisplay(1, versionMinor_);
      machine_->setDisplay(2, rpuMajor_);
      machine_->setDisplay(3, rpuMinor_);
      machine_->setDisplayCredits(machine_->getCredits(), true);
      machine_->setDisplayBallInPlay(0, true);
      lastHeadMode_ = 1;
   } else {
      lastHeadMode_ = 0;
   }
}

TopState AttractMode::update(unsigned long currentTime) {
   int returnState = MACHINE_STATE_ATTRACT;

   if ((currentTime - enterTime_) < 5000) {
      if (lastHeadMode_ != 1) {
         for (uint8_t i = 0; i < 4; i++) {
            if (savedNumPlayers_ > 0 && i >= savedNumPlayers_) machine_->setDisplayBlank(i, 0x00);
            else                                                machine_->setDisplay(i, savedScores_[i], true, 2);
         }
         for (uint8_t i = 0; i < 4; i++) machine_->setLampState(LAMP_PLAYER_1 + i, false);
         machine_->setDisplayCredits(machine_->getCredits(), true);
         machine_->setDisplayBallInPlay(0, true);
         lastHeadMode_ = 1;
      }
   } else {
      uint8_t headSlot = (uint8_t)((currentTime / 8000) % 3);
      if (headSlot == 0) {
         if (lastHeadMode_ != 2) {
            machine_->setLampState(LAMP_HIGH_SCORE_TO_DATE, true, 0, 250);
            machine_->setLampState(LAMP_GAME_OVER, false);
            for (uint8_t i = 0; i < 4; i++) machine_->setLampState(LAMP_PLAYER_1 + i, false);
            for (uint8_t i = 0; i < 4; i++) machine_->setDisplay(i, machine_->getHighScore(), true, 2);
            machine_->setDisplayBallInPlay(20, true);
         }
         lastHeadMode_ = 2;
      } else if (headSlot == 1) {
         if (lastHeadMode_ != 3) {
            machine_->setLampState(LAMP_HIGH_SCORE_TO_DATE, true, 0, 250);
            machine_->setLampState(LAMP_GAME_OVER, false);
            for (uint8_t i = 0; i < 4; i++) machine_->setLampState(LAMP_PLAYER_1 + i, false);
            for (uint8_t i = 0; i < 4; i++) machine_->setDisplay(i, machine_->getOriginalHighScore(), true, 2);
            machine_->setDisplayBallInPlay(1, true);
         }
         lastHeadMode_ = 3;
      } else {
         if (lastHeadMode_ != 4) {
            machine_->setLampState(LAMP_HIGH_SCORE_TO_DATE, false);
            machine_->setLampState(LAMP_GAME_OVER, true);
            machine_->setDisplayCredits(machine_->getCredits(), true);
            machine_->setDisplayBallInPlay(0, true);
            for (uint8_t i = 0; i < 4; i++) {
               if (savedNumPlayers_ > 0 && i >= savedNumPlayers_) machine_->setDisplayBlank(i, 0x00);
               else                                                machine_->setDisplay(i, savedScores_[i], true, 2);
            }
         }
         uint8_t activePlayer = (uint8_t)((currentTime / 250) % 4);
         for (uint8_t i = 0; i < 4; i++) machine_->setLampState(LAMP_PLAYER_1 + i, i == activePlayer);
         lastHeadMode_ = 4;
      }
   }

   if (lastPlayfieldMode_ != 1) {
      lastPlayfieldMode_ = 1;
      animCount_         = 0;
      animNum_           = 0;
      animStep_          = 0;
      lastAnimFrameTime_ = 0;
   }
   if ((currentTime - lastAnimFrameTime_) >= 100) {
      lastAnimFrameTime_ = currentTime;
      const uint8_t* animBytes = PeekAnimationBytes(animNum_, animStep_);
      machine_->setLampAnimationBytes(animBytes, NUM_LAMP_ANIMATION_BYTES);
      if (++animStep_ >= LAMP_ANIMATION_STEPS) {
         animStep_ = 0;
         animCount_++;
         if (animCount_ > REPEAT_LAMP_ANIMATIONS) {
            animCount_ = 0;
            if (++animNum_ >= NUM_LAMP_ANIMATIONS) {
               animNum_ = 0;
            }
         }
      }
   }

   uint8_t switchHit;
   while ((switchHit = machine_->pullFirstFromSwitchStack()) != PinballMachine::SWITCH_STACK_EMPTY) {
      if (switchHit == SW_CREDIT_RESET) {
         if (machine_->getCredits() >= 1 || machine_->getFreePlayMode()) {
            returnState = MACHINE_STATE_INIT_GAMEPLAY;
         }
      }
      if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
         machine_->addCoinToAudit(switchHit);
         machine_->addCredit(true, 1);
      }
      if (switchHit == SW_SELF_TEST_SWITCH &&
          (currentTime - machine_->getSelfTestChangedTime()) > 250) {
         machine_->setSelfTestChangedTime(currentTime);
         return TopState::HardwareTest;
      }
   }

   if (returnState == MACHINE_STATE_INIT_GAMEPLAY) return TopState::Game;
   return TopState::Attract;
}
