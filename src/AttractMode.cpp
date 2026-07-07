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

void AttractMode::enter(unsigned long) {
   machine_->playSoundCardEffect(0);
   machine_->disableSolenoidStack();
   machine_->turnOffAllLamps();
   machine_->setDisableFlippers(true);
   DEBUG_MESSAGE("Entering Attract Mode\n\r");
   lastPlayfieldMode_ = 0;

   if (versionMajor_ != 0) {
      // Show firmware version on all four score displays for the first
      // several seconds of attract mode before the normal display cycle begins.
      machine_->setDisplay(0, versionMajor_);
      machine_->setDisplay(1, versionMinor_);
      machine_->setDisplay(2, rpuMajor_);
      machine_->setDisplay(3, rpuMinor_);
      machine_->setDisplayCredits(settings_->credits, true);
      machine_->setDisplayBallInPlay(0, true);
      settings_->resetScoresToClearVersion = true;
      lastHeadMode_ = 1;   // suppress showPlayerScores until version display window expires
   } else {
      lastHeadMode_ = 0;
   }
}

TopState AttractMode::update(unsigned long currentTime) {
   game_->setCurrentTime(currentTime);
   int returnState = MACHINE_STATE_ATTRACT;

   if (currentTime < 16000) {
      if (lastHeadMode_ != 1) {
         game_->showPlayerScores(0xFF, false, false);
         game_->setPlayerLamps(0);
         machine_->setDisplayCredits(settings_->credits, true);
         machine_->setDisplayBallInPlay(0, true);
         lastHeadMode_ = 1;
      }
   } else if ((currentTime / 8000) % 2 == 0) {
      if (lastHeadMode_ != 2) {
         machine_->setLampState(LAMP_HIGH_SCORE_TO_DATE, true, 0, 250);
         machine_->setLampState(LAMP_GAME_OVER, false);
         game_->setPlayerLamps(0);
         game_->markScoreChanged(currentTime);
      }
      lastHeadMode_ = 2;
      game_->showPlayerScores(0xFF, false, false, settings_->highScore);
   } else {
      if (lastHeadMode_ != 3) {
         if (currentTime < 32000) {
            for (int count = 0; count < 4; count++) {
               game_->setScore(count, 0);
            }
            game_->setNumPlayers(0);
         }
         machine_->setLampState(LAMP_HIGH_SCORE_TO_DATE, false);
         machine_->setLampState(LAMP_GAME_OVER, true);
         machine_->setDisplayCredits(settings_->credits, true);
         machine_->setDisplayBallInPlay(0, true);
         game_->markScoreChanged(currentTime);
      }
      game_->showPlayerScores(0xFF, false, false);
      game_->setPlayerLamps(((currentTime / 250) % 4) + 1);
      lastHeadMode_ = 3;
   }

   if (lastPlayfieldMode_ != 1) {
      lastPlayfieldMode_ = 1;
      animNum_           = 0;
      animStep_          = 0;
      lastAnimFrameTime_ = 0;
   }
   if ((currentTime - lastAnimFrameTime_) >= 100) {
      lastAnimFrameTime_ = currentTime;
      machine_->setLampAnimation(animNum_, animStep_);
      if (++animStep_ >= LAMP_ANIMATION_STEPS) {
         animStep_ = 0;
         if (++animNum_ >= NUM_LAMP_ANIMATIONS) {
            animNum_ = 0;
         }
      }
   }

   uint8_t switchHit;
   while ((switchHit = machine_->pullFirstFromSwitchStack()) != PinballMachine::SWITCH_STACK_EMPTY) {
      if (switchHit == SW_CREDIT_RESET) {
         if (game_->addPlayer(true)) {
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
