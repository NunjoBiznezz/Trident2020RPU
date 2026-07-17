/**************************************************************************
 * TridentGame.cpp
 *
 * Game shell for Original Trident rules. Ball play and player rotation
 * are fully functional. Scoring and rule-specific switch handling are
 * stubs to be filled in later.
 **************************************************************************/

#include "TridentGame.h"
#include "SoundEffects.h"
#include "Trident.h"
#include "Trident2020.h"

static constexpr unsigned BUMPER_AWARD_3_BALL = 1000;
static constexpr unsigned BUMPER_AWARD_5_BALL = 100;
static constexpr unsigned SPINNER_AWARD = 200;
static constexpr unsigned SPINNER_AWARD_WITH_COLOR = 400;
static constexpr unsigned PURPLE_SPINNER_AWARD = 1000;
static constexpr unsigned STANDUP_TARGET_AWARD = 400;
static constexpr unsigned RIGHT_OUTLANE_SPECIAL_AWARD = 5000;
static constexpr unsigned MAXIMUM_BONUSES = 19;
static constexpr unsigned BONUS_AWARD = 1000;
static constexpr unsigned MAXIMUM_BONUS_AWARD = MAXIMUM_BONUSES * BONUS_AWARD;
static constexpr unsigned LEFT_INLANE_AWARD = 2000;
static constexpr unsigned RIGHT_INLANE_AWARD = 3000;
static constexpr unsigned RIGHT_OUTLANE_AWARD = 5000;
static constexpr unsigned ROLLOVER_AWARD = 100;
static constexpr unsigned LOWER_SLINGSHOT_AWARD = 100;
static constexpr unsigned UPPER_SLINGSHOT_AWARD = 1000;

static constexpr unsigned INITIAL_BONUS_VALUE = 1000;
static constexpr unsigned INITIAL_SAUCER_VALUE = 5000;
static constexpr unsigned INITIAL_ROLLOVER_BONUS = 2000;

/// Drop-target starting patterns for each multiplier level.
/// true = target raised (up), false = knocked down.
static constexpr uint8_t TWO_TARGETS_UP   = 0x0A;  // 01010 < 2X: targets 2 & 4 up.
static constexpr uint8_t THREE_TARGETS_UP = 0x15;  // 10101 < 3X: targets 1, 3 & 5 up.
static constexpr uint8_t FOUR_TARGETS_UP  = 0x1B;  // 11011 < 4X: targets 1, 2, 4 & 5 up.
static constexpr uint8_t FIVE_TARGETS_UP  = 0x1F;  // 11111 < 5X: all targets up.

TridentGame::TridentGame() {}

// ===========================================================================
// MachineMode interface
// ===========================================================================

void TridentGame::enter(unsigned long currentTime) {
   CurrentTime = currentTime;
   addPlayer(true);
   internalState_ = kInitGameplay;
   internalStateChanged_ = true;
}

TopState TridentGame::update(unsigned long currentTime) {
   CurrentTime = currentTime;

   bool curStateChanged = internalStateChanged_;
   internalStateChanged_ = false;
   int curState = internalState_;
   int returnState = curState;

   if (curState == kInitGameplay) {
      returnState = initGamePlay();
   } else if (curState == kInitNewBall) {
      returnState = initNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
   } else if (curState == kNormalGameplay) {
      showPlayerScores(CurrentPlayer, BallFirstSwitchHitTime == 0,
                       BallFirstSwitchHitTime > 0 && (CurrentTime - BallFirstSwitchHitTime) > 2000);
      // TODO: add Original Trident scoring and rule logic here
   } else if (curState == kBallOver) {
      CurrentScores[CurrentPlayer] = CurrentPlayerCurrentScore;
      if (SamePlayerShootsAgain) {
         returnState = kInitNewBall;
      } else {
         CurrentPlayer += 1;
         if (CurrentPlayer >= CurrentNumPlayers) {
            CurrentPlayer = 0;
            CurrentBallInPlay += 1;
         }
         CurrentPlayerCurrentScore = CurrentScores[CurrentPlayer];
         if (CurrentBallInPlay > settings_->tridentSettings.ballsPerGame) {
            machine_->playSoundEffect(SOUND_EFFECT_GAME_OVER);
            setPlayerLamps(0);
            machine_->setLastGameResult(CurrentNumPlayers, CurrentScores);
            for (int count = 0; count < CurrentNumPlayers; count++) {
               machine_->setDisplay(count, CurrentScores[count], true, 2);
            }
            returnState = kMatchMode;
         } else {
            returnState = kInitNewBall;
         }
      }
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
            break;

         case SW_CREDIT_RESET:
            if (curState == kNormalGameplay) {
               addPlayer(false);
            }
            break;

         case SW_OUTHOLE:
            if (curState == kNormalGameplay) {
               if (!BallSaveUsed && settings_->ballSaveNumSeconds > 0 && BallFirstSwitchHitTime != 0 &&
                   (CurrentTime - BallFirstSwitchHitTime) < ((unsigned long)settings_->ballSaveNumSeconds * 1000)) {
                  machine_->pushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
                  BallSaveUsed = true;
               } else {
                  returnState = kBallOver;
               }
            }
            break;

         default:
            if (curState == kNormalGameplay && BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;
         }
      }
   }

   if (returnState < 0) {
      return TopState::HardwareTest;
   }
   if (returnState == kMatchMode) {
      return TopState::Match;
   }
   if (returnState != curState) {
      internalState_ = returnState;
      internalStateChanged_ = true;
   }
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
      machine_->setLampState(LAMP_PLAYER_1 + playerOffset + i, (numPlayers == (i + 1)) ? 1 : 0, 0, (uint16_t)flashPeriod);
   }
}

void TridentGame::showPlayerScores(uint8_t displayToUpdate, bool flashCurrent, bool dashCurrent, unsigned long allScoresShowValue) {
   (void)allScoresShowValue;
   for (uint8_t n = 0; n < 4; n++) {
      if (displayToUpdate != 0xFF && displayToUpdate != n) {
         continue;
      }
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
      CurrentScores[count] = 0;
      SamePlayerShootsAgain = false;
   }

   CurrentBallInPlay = 1;
   CurrentNumPlayers = 1;
   CurrentPlayer = 0;
   showPlayerScores(0xFF, false, false);

   if (machine_->readSingleSwitchState(SW_SAUCER)) {
      machine_->pushToSolenoidStack(SOL_SAUCER, 5);
   }

   return kInitNewBall;
}

int TridentGame::initNewBall(bool curStateChanged, uint8_t playerNum, int ballNum) {
   if (curStateChanged) {
      SamePlayerShootsAgain = false;
      BallFirstSwitchHitTime = 0;
      BallSaveUsed = false;
      NumTiltWarnings = 0;
      LastTiltWarningTime = 0;

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
      return kInitNewBall;
   }
   return kNormalGameplay;
}

void TridentGame::startBallBackgroundSong(uint8_t ballNum) {
   uint8_t song;
   if (ballNum == 1) {
      song = SOUND_EFFECT_BACKGROUND_1;
   } else if (ballNum == settings_->tridentSettings.ballsPerGame) {
      song = SOUND_EFFECT_BACKGROUND_6;
   } else {
      song = SOUND_EFFECT_BACKGROUND_2 + (uint8_t)(CurrentTime % 4);
   }
   machine_->playBackgroundSong(song);
}


