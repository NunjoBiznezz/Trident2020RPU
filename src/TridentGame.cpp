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
static constexpr unsigned STANDUP_TARGET_AWARD = 1000;
static constexpr unsigned RIGHT_OUTLANE_SPECIAL_AWARD = 5000;
static constexpr unsigned MAXIMUM_BONUSES = 19;
static constexpr unsigned BONUS_AWARD = 1000;
static constexpr unsigned MAXIMUM_BONUS_AWARD = MAXIMUM_BONUSES * BONUS_AWARD;
static constexpr unsigned long BONUS_COUNTDOWN_DELAY = 100;

// Indexed by (switchHit - SW_PURPLE): PURPLE=0, YELLOW=1, AMBER=2, GREEN=3, WHITE=4
static constexpr uint8_t StandupTargetLamps[] = {
   LAMP_STAND_UP_PURPLE, LAMP_STAND_UP_YELLOW, LAMP_STAND_UP_AMBER,
   LAMP_STAND_UP_GREEN,  LAMP_STAND_UP_WHITE
};

// Spinner lamp lit when the corresponding standup target is first hit this ball.
static constexpr uint8_t SpinnerLamps[] = {
   LAMP_LEFT_SPINNER_PURPLE,  // tIdx 0 = PURPLE
   LAMP_RIGHT_SPINNER_YELLOW, // tIdx 1 = YELLOW
   LAMP_LEFT_SPINNER_AMBER,   // tIdx 2 = AMBER
   LAMP_RIGHT_SPINNER_GREEN,  // tIdx 3 = GREEN
   LAMP_LEFT_SPINNER_WHITE,   // tIdx 4 = WHITE
};
// Which spinner gets +400 per target: 0=none, 1=left, 2=right
static constexpr uint8_t SpinnerSide[] = { 0, 2, 1, 2, 1 };

static constexpr unsigned long LeftLaneValues[] = { 2000, 4000, 6000, 8000 };
static constexpr uint8_t      LeftLaneLamps[]  = { LAMP_LEFT_LANE_2K, LAMP_LEFT_LANE_4K, LAMP_LEFT_LANE_6K, LAMP_LEFT_LANE_8K };

static constexpr uint8_t DropTargetSolenoidArray[] = {
   SOL_DROP_TARGET_1, SOL_DROP_TARGET_2, SOL_DROP_TARGET_3, SOL_DROP_TARGET_4, SOL_DROP_TARGET_5
};
static constexpr unsigned LEFT_INLANE_AWARD = 2000;
static constexpr unsigned RIGHT_INLANE_AWARD = 3000;
static constexpr unsigned RIGHT_OUTLANE_AWARD = 5000;
static constexpr unsigned ROLLOVER_AWARD = 100;
static constexpr unsigned LOWER_SLINGSHOT_AWARD = 10;
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
      if (curStateChanged) {
         machine_->disableSolenoidStack();
         machine_->setDisableFlippers(true);
         SavedBonusValue = CurrentBonusValue;
         BonusCountdownPassesLeft = BonusMultiplier;
         BonusCountdownTime = CurrentTime + BONUS_COUNTDOWN_DELAY;
      }
      if (CurrentBonusValue > 0 && CurrentTime >= BonusCountdownTime) {
         CurrentPlayerCurrentScore += BONUS_AWARD;
         CurrentBonusValue -= 1;
         showBonusLamps();
         machine_->playSoundEffect(SOUND_EFFECT_BONUS_COUNT);
         showPlayerScores(CurrentPlayer, false, false);
         BonusCountdownTime += BONUS_COUNTDOWN_DELAY;
      }
      if (CurrentBonusValue == 0) {
         BonusCountdownPassesLeft -= 1;
         if (BonusCountdownPassesLeft > 0) {
            CurrentBonusValue = SavedBonusValue;
            showBonusLamps();
            BonusCountdownTime = CurrentTime + BONUS_COUNTDOWN_DELAY;
         } else {
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

         case SW_SAUCER:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               CurrentPlayerCurrentScore += CurrentSaucerValue;
               uint8_t saucerSound;
               if      (CurrentSaucerValue >= 30000) saucerSound = SOUND_EFFECT_SAUCER_HIT_30K;
               else if (CurrentSaucerValue >= 20000) saucerSound = SOUND_EFFECT_SAUCER_HIT_20K;
               else if (CurrentSaucerValue >= 10000) saucerSound = SOUND_EFFECT_SAUCER_HIT_10K;
               else                                  saucerSound = SOUND_EFFECT_SAUCER_HIT_5K;
               machine_->playSoundEffect(saucerSound);
               machine_->pushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + 1000);
            }
            break;

         case SW_WHITE:
         case SW_GREEN:
         case SW_AMBER:
         case SW_YELLOW:
         case SW_PURPLE: {
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               advanceBonus(1);
               CurrentPlayerCurrentScore += STANDUP_TARGET_AWARD;
               uint8_t tIdx = switchHit - SW_PURPLE;
               if (!(SpinnerBonusMask & (1u << tIdx))) {
                  SpinnerBonusMask |= (1u << tIdx);
                  machine_->setLampState(SpinnerLamps[tIdx], true);
                  if (SpinnerSide[tIdx] == 1) LeftSpinnerValue  += SPINNER_AWARD_WITH_COLOR;
                  if (SpinnerSide[tIdx] == 2) RightSpinnerValue += SPINNER_AWARD_WITH_COLOR;
               }
               if (!(StandupTargetsMask & (1u << tIdx))) {
                  StandupTargetsMask |= (1u << tIdx);
                  machine_->setLampState(StandupTargetLamps[tIdx], true);
                  if (StandupTargetsMask == 0x1F) {
                     for (uint8_t i = 0; i < 5; i++) {
                        machine_->setLampState(StandupTargetLamps[i], false);
                     }
                     StandupTargetsMask = 0;
                     StandupCompletions += 1;
                     machine_->playSoundEffect(SOUND_EFFECT_STANDUPS_CLEARED);
                     if (StandupCompletions == 1) {
                        StandupExtraBallAvailable = true;
                        machine_->setLampState(LAMP_EXTRA_BALL,       true);
                        machine_->setLampState(LAMP_STAND_UP_SPECIAL, true);
                     } else {
                        machine_->setLampState(LAMP_STAND_UP_SPECIAL, false);
                        if (settings_->tournamentScoring) {
                           CurrentPlayerCurrentScore += settings_->tridentSettings.specialValue;
                        } else {
                           machine_->addSpecialCredit();
                        }
                     }
                  }
               }
            }
            break;
         }

         case SW_UL_SLING:
         case SW_UR_SLING:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               CurrentPlayerCurrentScore += UPPER_SLINGSHOT_AWARD;
               advanceBonus(1);
            }
            break;

         case SW_LL_SLING:
         case SW_LR_SLING:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               CurrentPlayerCurrentScore += LOWER_SLINGSHOT_AWARD;
               advanceBonus(1);
            }
            break;

         case SW_ROLLOVER:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               CurrentPlayerCurrentScore += ROLLOVER_AWARD;
               if (LeftLaneValueIndex < 3) {
                  machine_->setLampState(LeftLaneLamps[LeftLaneValueIndex], false);
                  LeftLaneValueIndex += 1;
                  machine_->setLampState(LeftLaneLamps[LeftLaneValueIndex], true);
               }
            }
            break;

         case SW_LEFT_INLANE:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               advanceBonus(1);
               CurrentPlayerCurrentScore += LeftLaneValues[LeftLaneValueIndex];
            }
            break;

         case SW_RIGHT_INLANE:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               CurrentPlayerCurrentScore += RIGHT_INLANE_AWARD;
               advanceBonus(3);
               if (StandupExtraBallAvailable) {
                  StandupExtraBallAvailable = false;
                  machine_->setLampState(LAMP_EXTRA_BALL, false);
                  machine_->setLampState(LAMP_SHOOT_AGAIN, true);
                  machine_->playSoundEffect(SOUND_EFFECT_EXTRA_BALL);
                  SamePlayerShootsAgain = true;
               }
            }
            break;

         case SW_RIGHT_OUTLANE:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               CurrentPlayerCurrentScore += RIGHT_OUTLANE_AWARD;
               advanceBonus(3);
               if (DropTargetSpecialAvailable) {
                  DropTargetSpecialAvailable = false;
                  machine_->setLampState(LAMP_RIGHT_OUTLANE_SPECIAL, false);
                  machine_->setLampState(LAMP_DROP_TARGET_SPECIAL, false);
                  if (settings_->tournamentScoring) {
                     CurrentPlayerCurrentScore += settings_->tridentSettings.specialValue;
                  } else {
                     machine_->addSpecialCredit();
                  }
               }
            }
            break;

         case SW_DROP_TARGET_1:
         case SW_DROP_TARGET_2:
         case SW_DROP_TARGET_3:
         case SW_DROP_TARGET_4:
         case SW_DROP_TARGET_5: {
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               uint8_t targetIdx = SW_DROP_TARGET_1 - switchHit;
               if (DropTargetsMask & (1u << targetIdx)) {
                  DropTargetsMask &= ~(1u << targetIdx);
                  machine_->setLampState(DropTargetLampArray[targetIdx], false);
                  if (DropTargetsMask == 0) {
                     if (BonusMultiplier < 5) {
                        BonusMultiplier += 1;
                     }
                     switch (BonusMultiplier) {
                     case 2:
                        machine_->setLampState(LAMP_BONUS_2X_FEATURE, false);
                        machine_->setLampState(LAMP_BONUS_2X, true);
                        CurrentSaucerValue = 10000;
                        machine_->setLampState(LAMP_TOP_EJECT_5K, false);
                        machine_->setLampState(LAMP_TOP_EJECT_10K, true);
                        setupDropTargets(THREE_TARGETS_UP);
                        machine_->setLampState(LAMP_BONUS_3X_FEATURE, true);
                        break;
                     case 3:
                        machine_->setLampState(LAMP_BONUS_3X_FEATURE, false);
                        machine_->setLampState(LAMP_BONUS_3X, true);
                        CurrentSaucerValue = 20000;
                        machine_->setLampState(LAMP_TOP_EJECT_10K, false);
                        machine_->setLampState(LAMP_TOP_EJECT_20K, true);
                        setupDropTargets(FOUR_TARGETS_UP);
                        machine_->setLampState(LAMP_BONUS_4X_FEATURE, true);
                        break;
                     case 4:
                        machine_->setLampState(LAMP_BONUS_4X_FEATURE, false);
                        machine_->setLampState(LAMP_BONUS_4X, true);
                        CurrentSaucerValue = 30000;
                        machine_->setLampState(LAMP_TOP_EJECT_20K, false);
                        machine_->setLampState(LAMP_TOP_EJECT_30K, true);
                        setupDropTargets(FIVE_TARGETS_UP);
                        machine_->setLampState(LAMP_BONUS_5X_FEATURE, true);
                        break;
                     case 5:
                        machine_->setLampState(LAMP_BONUS_5X_FEATURE, false);
                        machine_->setLampState(LAMP_BONUS_5X, true);
                        setupDropTargets(FIVE_TARGETS_UP);
                        break;
                     }
                     if (BonusMultiplier >= settings_->tridentSettings.dropTargetSpecialAt) {
                        DropTargetSpecialAvailable = true;
                        machine_->setLampState(LAMP_RIGHT_OUTLANE_SPECIAL, true);
                        machine_->setLampState(LAMP_DROP_TARGET_SPECIAL, true);
                     }
                  }
               }
            }
            break;
         }

         case SW_TOP_BUMPER:
         case SW_BOTTOM_BUMPER:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               unsigned bumperAward = (settings_->tridentSettings.ballsPerGame == 3) ? BUMPER_AWARD_3_BALL : BUMPER_AWARD_5_BALL;
               CurrentPlayerCurrentScore += bumperAward;
            }
            break;

         case SW_LEFT_SPINNER:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               CurrentPlayerCurrentScore += LeftSpinnerValue;
            }
            break;

         case SW_RIGHT_SPINNER:
            if (curState == kNormalGameplay) {
               if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
               CurrentPlayerCurrentScore += RightSpinnerValue;
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

      // Bonus and multiplier reset
      BonusMultiplier = 1;
      CurrentBonusValue = 1;
      showBonusLamps();
      machine_->setLampState(LAMP_BONUS_2X_FEATURE, true);
      machine_->setLampState(LAMP_BONUS_2X, false);
      machine_->setLampState(LAMP_BONUS_3X_FEATURE, false);
      machine_->setLampState(LAMP_BONUS_3X, false);
      machine_->setLampState(LAMP_BONUS_4X_FEATURE, false);
      machine_->setLampState(LAMP_BONUS_4X, false);
      machine_->setLampState(LAMP_BONUS_5X_FEATURE, false);
      machine_->setLampState(LAMP_BONUS_5X, false);

      // Standup targets: all off, no completions
      StandupTargetsMask       = 0;
      StandupCompletions       = 0;
      StandupExtraBallAvailable = false;
      for (uint8_t i = 0; i < 5; i++) {
         machine_->setLampState(StandupTargetLamps[i], false);
      }
      machine_->setLampState(LAMP_EXTRA_BALL,       false);
      machine_->setLampState(LAMP_STAND_UP_SPECIAL, false);

      // Spinner lamps and values: reset all
      SpinnerBonusMask  = 0;
      LeftSpinnerValue  = SPINNER_AWARD;
      RightSpinnerValue = SPINNER_AWARD;
      machine_->setLampState(LAMP_LEFT_SPINNER_WHITE,   false);
      machine_->setLampState(LAMP_LEFT_SPINNER_AMBER,   false);
      machine_->setLampState(LAMP_LEFT_SPINNER_PURPLE,  false);
      machine_->setLampState(LAMP_RIGHT_SPINNER_YELLOW, false);
      machine_->setLampState(LAMP_RIGHT_SPINNER_GREEN,  false);
      machine_->setLampState(LAMP_RIGHT_SPINNER_PURPLE, false);

      // Left lane value: reset to 2K
      LeftLaneValueIndex = 0;
      for (uint8_t i = 0; i < 4; i++) {
         machine_->setLampState(LeftLaneLamps[i], i == 0);
      }

      // Drop targets start at 2X: targets 2 and 4 up
      DropTargetSpecialAvailable = false;
      machine_->setLampState(LAMP_RIGHT_OUTLANE_SPECIAL, false);
      machine_->setLampState(LAMP_DROP_TARGET_SPECIAL, false);
      setupDropTargets(TWO_TARGETS_UP);

      // Saucer starts at 5K
      CurrentSaucerValue = INITIAL_SAUCER_VALUE;
      machine_->setLampState(LAMP_TOP_EJECT_5K,  true);
      machine_->setLampState(LAMP_TOP_EJECT_10K, false);
      machine_->setLampState(LAMP_TOP_EJECT_20K, false);
      machine_->setLampState(LAMP_TOP_EJECT_30K, false);

      if (machine_->readSingleSwitchState(SW_SAUCER)) {
         machine_->pushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + 500);
      }
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

void TridentGame::advanceBonus(uint8_t positions) {
   uint8_t newBonus = CurrentBonusValue + positions;
   if (newBonus > MAXIMUM_BONUSES) newBonus = MAXIMUM_BONUSES;
   CurrentBonusValue = newBonus;
   showBonusLamps();
}

void TridentGame::setupDropTargets(uint8_t mask) {
   DropTargetsMask = mask;
   machine_->pushToSolenoidStack(SOL_DROP_TARGET_RESET, 10);
   for (uint8_t i = 0; i < 5; i++) {
      bool targetUp = (mask >> i) & 1;
      if (!targetUp) {
         machine_->pushToTimedSolenoidStack(DropTargetSolenoidArray[i], 4, CurrentTime + 500);
      }
      machine_->setLampState(DropTargetLampArray[i], targetUp);
   }
}

void TridentGame::showBonusLamps() {
   for (uint8_t i = 0; i < 10; i++) {
      machine_->setLampState(LAMP_BONUS_1 + i, false);
   }
   if (CurrentBonusValue == 0) return;
   if (CurrentBonusValue >= 10) {
      machine_->setLampState(LAMP_BONUS_10, true);
      if (CurrentBonusValue > 10) {
         machine_->setLampState(LAMP_BONUS_1 + (CurrentBonusValue - 11), true);
      }
   } else {
      machine_->setLampState(LAMP_BONUS_1 + (CurrentBonusValue - 1), true);
   }
}
