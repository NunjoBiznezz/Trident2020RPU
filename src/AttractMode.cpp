/**************************************************************************
 * AttractMode.cpp
 **************************************************************************/

#include "AttractMode.h"
#include "MachineState.h"
#include "RPU.h"
#include "SelfTestAndAudit.h"
#include "Trident.h"
#include "Trident2020.h"

#if defined(DEBUG_MESSAGES) && defined(DEBUG_PORT)
#  define DEBUG_MESSAGE(x) DEBUG_PORT.write(x)
#else
#  define DEBUG_MESSAGE(x)
#endif

void AttractMode::enter(unsigned long) {
#ifdef RPU_OS_USE_SB100
   RPU_PlaySB100(0);
#endif
   RPU_DisableSolenoidStack();
   RPU_TurnOffAllLamps();
   RPU_SetDisableFlippers(true);
   DEBUG_MESSAGE("Entering Attract Mode\n\r");
   lastHeadMode_      = 0;
   lastPlayfieldMode_ = 0;
}

TopState AttractMode::update(unsigned long currentTime) {
   int returnState = MACHINE_STATE_ATTRACT;

   if (currentTime < 16000) {
      if (lastHeadMode_ != 1) {
         game_->showPlayerScores(0xFF, false, false);
         game_->setPlayerLamps(0);
         RPU_SetDisplayCredits(*ctx_->credits, true);
         RPU_SetDisplayBallInPlay(0, true);
      }
   } else if ((currentTime / 8000) % 2 == 0) {
      if (lastHeadMode_ != 2) {
         RPU_SetLampState(HIGH_SCORE_TO_DATE, true, 0, 250);
         RPU_SetLampState(GAME_OVER, false);
         game_->setPlayerLamps(0);
         game_->markScoreChanged(currentTime);
      }
      lastHeadMode_ = 2;
      game_->showPlayerScores(0xFF, false, false, *ctx_->highScore);
   } else {
      if (lastHeadMode_ != 3) {
         if (currentTime < 32000) {
            for (int count = 0; count < 4; count++) {
               game_->setScore(count, 0);
            }
            game_->setNumPlayers(0);
         }
         RPU_SetLampState(HIGH_SCORE_TO_DATE, false);
         RPU_SetLampState(GAME_OVER, true);
         RPU_SetDisplayCredits(*ctx_->credits, true);
         RPU_SetDisplayBallInPlay(0, true);
         game_->markScoreChanged(currentTime);
      }
      game_->showPlayerScores(0xFF, false, false);
      game_->setPlayerLamps(((currentTime / 250) % 4) + 1);
      lastHeadMode_ = 3;
   }

   if ((currentTime / 10000) % 3 < 2) {
      if (lastPlayfieldMode_ != 1) {
         RPU_TurnOffAllLamps();
         game_->setGameMode(GAME_MODE_SKILL_SHOT);
      }
      game_->showSaucerLamps();
      game_->showDropTargetLamps();
      game_->showStandupTargetLamps();
      game_->showLeftSpinnerLamps();
      game_->showLeftLaneLamps();
      lastPlayfieldMode_ = 1;
   } else {
      if (lastPlayfieldMode_ != 2) {
         RPU_TurnOffAllLamps();
         lastLadderBonus_ = 1;
         lastLadderTime_  = currentTime;
      }
      if ((currentTime - lastLadderTime_) > 200) {
         lastLadderBonus_ += 1;
         lastLadderTime_   = currentTime;
         game_->showBonusOnTree(lastLadderBonus_ % MAX_DISPLAY_BONUS);
      }
      lastPlayfieldMode_ = 2;
   }

   uint8_t switchHit;
   while ((switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY) {
      if (switchHit == SW_CREDIT_RESET) {
         if (game_->addPlayer(true, *ctx_)) {
            returnState = MACHINE_STATE_INIT_GAMEPLAY;
         }
      }
      if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
         machine_->addCoinToAudit(switchHit);
         machine_->addCredit(true, 1);
      }
      if (switchHit == SW_SELF_TEST_SWITCH &&
          (currentTime - GetLastSelfTestChangedTime()) > 250) {
         returnState = MACHINE_STATE_TEST_LAMPS;
         SetLastSelfTestChangedTime(currentTime);
      }
   }

   if (returnState < 0)                         return TopState::SelfTest;
   if (returnState == MACHINE_STATE_INIT_GAMEPLAY) return TopState::Game;
   return TopState::Attract;
}
