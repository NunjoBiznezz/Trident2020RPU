/**************************************************************************
 * TridentGame.cpp
 *
 * Game shell for Original Trident rules. Ball play and player rotation
 * are fully functional. Scoring and rule-specific switch handling are
 * stubs to be filled in later.
 **************************************************************************/

#include "TridentGame.h"
#include "MachineState.h"
#include "SoundEffects.h"
#include "Trident.h"
#include "Trident2020.h"

TridentGame::TridentGame() {}

// ===========================================================================
// MachineMode interface
// ===========================================================================

void TridentGame::enter(unsigned long currentTime) {
   CurrentTime = currentTime;
   addPlayer(true);
   internalState_        = MACHINE_STATE_INIT_GAMEPLAY;
   internalStateChanged_ = true;
}

TopState TridentGame::update(unsigned long currentTime) {
   CurrentTime = currentTime;

   bool curStateChanged  = internalStateChanged_;
   internalStateChanged_ = false;
   int  curState         = internalState_;
   int  returnState      = curState;

   if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
      returnState = initGamePlay();
   } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
      returnState = initNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
   } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
      showPlayerScores(CurrentPlayer, BallFirstSwitchHitTime == 0,
                       BallFirstSwitchHitTime > 0 && (CurrentTime - BallFirstSwitchHitTime) > 2000);
      // TODO: add Original Trident scoring and rule logic here
   } else if (curState == MACHINE_STATE_BALL_OVER) {
      CurrentScores[CurrentPlayer] = CurrentPlayerCurrentScore;
      if (SamePlayerShootsAgain) {
         returnState = MACHINE_STATE_INIT_NEW_BALL;
      } else {
         CurrentPlayer += 1;
         if (CurrentPlayer >= CurrentNumPlayers) {
            CurrentPlayer = 0;
            CurrentBallInPlay += 1;
         }
         CurrentPlayerCurrentScore = CurrentScores[CurrentPlayer];
         if (CurrentBallInPlay > settings_->ballsPerGame) {
            machine_->playSoundEffect(SOUND_EFFECT_GAME_OVER);
            setPlayerLamps(0);
            for (int count = 0; count < CurrentNumPlayers; count++) {
               machine_->setDisplay(count, CurrentScores[count], true, 2);
            }
            returnState = MACHINE_STATE_MATCH_MODE;
         } else {
            returnState = MACHINE_STATE_INIT_NEW_BALL;
         }
      }
   } else if (curState == MACHINE_STATE_MATCH_MODE) {
      returnState = showMatchSequence(curStateChanged);
   }

   // Switch stack — drain every tick regardless of state
   if (NumTiltWarnings <= settings_->maxTiltWarnings) {
      uint8_t switchHit;
      while ((switchHit = machine_->pullFirstFromSwitchStack()) != PinballMachine::SWITCH_STACK_EMPTY) {
         switch (switchHit) {
         case SW_SLAM:
            break;

         case SW_TILT:
            if ((CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
               LastTiltWarningTime = CurrentTime;
               NumTiltWarnings += 1;
               if (NumTiltWarnings > settings_->maxTiltWarnings) {
                  machine_->disableSolenoidStack();
                  machine_->setDisableFlippers(true);
                  machine_->turnOffAllLamps();
                  machine_->setLampState(LAMP_TILT, true);
               }
               machine_->playSoundEffect(SOUND_EFFECT_TILT_WARNING);
            }
            break;

         case SW_SELF_TEST_SWITCH:
            returnState = -1;
            machine_->setSelfTestChangedTime(CurrentTime);
            break;

         case SW_CREDIT_RESET:
            if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
               addPlayer(false);
            }
            break;

         case SW_OUTHOLE:
            if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
               if (!BallSaveUsed && settings_->ballSaveNumSeconds > 0 &&
                   BallFirstSwitchHitTime != 0 &&
                   (CurrentTime - BallFirstSwitchHitTime) < ((unsigned long)settings_->ballSaveNumSeconds * 1000)) {
                  machine_->pushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
                  BallSaveUsed = true;
               } else {
                  returnState = MACHINE_STATE_BALL_OVER;
               }
            }
            break;

         default:
            if (curState == MACHINE_STATE_NORMAL_GAMEPLAY && BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;
         }
      }
   }

   if (returnState != curState) {
      internalState_        = returnState;
      internalStateChanged_ = true;
   }

   if (returnState < 0) return TopState::HardwareTest;
   if (returnState == MACHINE_STATE_ATTRACT) return TopState::Attract;
   return TopState::Game;
}

// ===========================================================================
// Public helpers
// ===========================================================================

bool TridentGame::addPlayer(bool resetNumPlayers) {
   if (!machine_->canAddPlayer()) {
      return false;
   }

   if (resetNumPlayers) {
      CurrentNumPlayers = 0;
   }
   if (CurrentNumPlayers >= 4) {
      return false;
   }

   CurrentNumPlayers += 1;
   machine_->setDisplay(CurrentNumPlayers - 1, 0);
   machine_->setDisplayBlank(CurrentNumPlayers - 1, 0x30);

   if (!settings_->freePlayMode) {
      machine_->decrementCredits();
      machine_->setDisplayCredits(machine_->getCredits());
      machine_->setCoinLockout(false);
   }

   machine_->playSoundEffect(SOUND_EFFECT_ADD_PLAYER_1 + (CurrentNumPlayers - 1));
   setPlayerLamps(CurrentNumPlayers);
   machine_->recordGamePlayed();
   return true;
}

// ===========================================================================
// Private helpers
// ===========================================================================

void TridentGame::setPlayerLamps(uint8_t numPlayers, uint8_t playerOffset, int flashPeriod) {
   for (uint8_t i = 0; i < 4; i++) {
      machine_->setLampState(LAMP_PLAYER_1 + playerOffset + i,
                             (numPlayers == (i + 1)) ? 1 : 0, 0, (uint16_t)flashPeriod);
   }
}

void TridentGame::showPlayerScores(uint8_t displayToUpdate, bool flashCurrent, bool dashCurrent,
                                    unsigned long allScoresShowValue) {
   (void)allScoresShowValue;
   for (uint8_t n = 0; n < 4; n++) {
      if (displayToUpdate != 0xFF && displayToUpdate != n) continue;
      if (displayToUpdate == 0xFF && n >= CurrentNumPlayers) {
         machine_->setDisplayBlank(n, 0x00);
         continue;
      }
      unsigned long score = (n == CurrentPlayer) ? CurrentPlayerCurrentScore : CurrentScores[n];
      if (flashCurrent || dashCurrent) {
         machine_->setDisplayFlash(n, score, 500);
      } else {
         machine_->setDisplay(n, score, true, 2);
      }
   }
}

int TridentGame::initGamePlay() {
   machine_->enableSolenoidStack();
   machine_->setCoinLockout(settings_->credits >= settings_->maximumCredits);
   machine_->turnOffAllLamps();
   setPlayerLamps(1);

   for (int count = 0; count < 4; count++) {
      machine_->setDisplay(count, 0);
      machine_->setDisplayBlank(count, (count == 0) ? 0x30 : 0x00);
      CurrentScores[count]  = 0;
      SamePlayerShootsAgain = false;
   }

   CurrentBallInPlay = 1;
   CurrentNumPlayers = 1;
   CurrentPlayer     = 0;
   showPlayerScores(0xFF, false, false);

   if (machine_->readSingleSwitchState(SW_SAUCER)) {
      machine_->pushToSolenoidStack(SOL_SAUCER, 5);
   }

   return MACHINE_STATE_INIT_NEW_BALL;
}

int TridentGame::initNewBall(bool curStateChanged, uint8_t playerNum, int ballNum) {
   if (curStateChanged) {
      SamePlayerShootsAgain  = false;
      BallFirstSwitchHitTime = 0;
      BallSaveUsed           = false;
      NumTiltWarnings        = 0;
      LastTiltWarningTime    = 0;

      machine_->setDisableFlippers(false);
      machine_->enableSolenoidStack();
      machine_->setDisplayCredits(settings_->credits, true);
      setPlayerLamps(playerNum + 1, 4);

      if (CurrentNumPlayers > 1 && (ballNum != 1 || playerNum != 0)) {
         machine_->playSoundEffect(SOUND_EFFECT_PLAYER_1_UP + playerNum);
      }
      startBallBackgroundSong(ballNum);

      machine_->setDisplayBallInPlay(ballNum);
      machine_->setLampState(LAMP_BALL_IN_PLAY, true);
      machine_->setLampState(LAMP_TILT, false);

      if (settings_->ballSaveNumSeconds > 0) {
         machine_->setLampState(LAMP_SHOOT_AGAIN, true, 0, 500);
      }

      CurrentPlayerCurrentScore = CurrentScores[CurrentPlayer];

      if (machine_->readSingleSwitchState(SW_OUTHOLE)) {
         machine_->pushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
      }
   }

   if (machine_->readSingleSwitchState(SW_OUTHOLE)) {
      return MACHINE_STATE_INIT_NEW_BALL;
   }
   return MACHINE_STATE_NORMAL_GAMEPLAY;
}

void TridentGame::startBallBackgroundSong(uint8_t ballNum) {
   uint8_t song;
   if (ballNum == 1)                       song = SOUND_EFFECT_BACKGROUND_1;
   else if (ballNum == settings_->ballsPerGame) song = SOUND_EFFECT_BACKGROUND_6;
   else                                    song = SOUND_EFFECT_BACKGROUND_2 + (uint8_t)(CurrentTime % 4);
   machine_->playBackgroundSong(song);
}

int TridentGame::showMatchSequence(bool curStateChanged) {
   if (!settings_->matchFeature) {
      return MACHINE_STATE_ATTRACT;
   }

   if (curStateChanged) {
      MatchSequenceStartTime = CurrentTime;
      MatchDelay             = 1500;
      MatchDigit             = (uint8_t)(CurrentTime % 10);
      NumMatchSpins          = 0;
      ScoreMatches           = 0;
      machine_->setLampState(LAMP_MATCH, true, 0);
      machine_->setDisableFlippers(true);
      machine_->setLampState(LAMP_BALL_IN_PLAY, false);
   }

   if (NumMatchSpins < 40) {
      if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
         MatchDigit += 1;
         if (MatchDigit > 9) MatchDigit = 0;
         machine_->playSoundEffect(SOUND_EFFECT_MATCH_SPIN);
         machine_->setDisplayBallInPlay((int)MatchDigit * 10);
         MatchDelay += 50 + 4 * NumMatchSpins;
         NumMatchSpins += 1;
         machine_->setLampState(LAMP_MATCH, (NumMatchSpins % 2) != 0, 0);
         if (NumMatchSpins == 40) {
            machine_->setLampState(LAMP_MATCH, false);
            MatchDelay = CurrentTime - MatchSequenceStartTime;
         }
      }
   }

   if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
      if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
         if ((CurrentNumPlayers > (NumMatchSpins - 40)) &&
             ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
            ScoreMatches |= (1 << (NumMatchSpins - 40));
            machine_->addSpecialCredit();
            MatchDelay += 1000;
            NumMatchSpins += 1;
            machine_->setLampState(LAMP_MATCH, true);
         } else {
            NumMatchSpins += 1;
         }
         if (NumMatchSpins == 44) {
            MatchDelay += 5000;
         }
      }
   }

   if (NumMatchSpins > 43) {
      if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
         return MACHINE_STATE_ATTRACT;
      }
   }

   for (int count = 0; count < 4; count++) {
      if ((ScoreMatches >> count) & 0x01) {
         if ((CurrentTime / 200) % 2 != 0) {
            machine_->setDisplayBlank(count, machine_->getDisplayBlank(count) & 0x0F);
         } else {
            machine_->setDisplayBlank(count, machine_->getDisplayBlank(count) | 0x30);
         }
      }
   }

   return MACHINE_STATE_MATCH_MODE;
}
