/**************************************************************************
 * Trident2020Game.cpp
 *
 * Implements the Trident 2020 game rules class. All gameplay state and
 * logic is encapsulated here; machine-level settings are accessed via
 * the MachineSettings reference set by setSettings().
 **************************************************************************/

#include "Trident2020Game.h"
#include "MachineState.h"
#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"


#if defined(DEBUG_MESSAGES) && defined(DEBUG_PORT)
#  define DEBUG_MESSAGE(x) DEBUG_PORT.write(x)
#else
#  define DEBUG_MESSAGE(x)
#endif

// ---------------------------------------------------------------------------
// File-local arrays used by lamp display methods (moved from main.cpp)
// ---------------------------------------------------------------------------
static const uint8_t DropTargetLampArray[]   = {LAMP_DROP_TARGET_1, LAMP_DROP_TARGET_2, LAMP_DROP_TARGET_3, LAMP_DROP_TARGET_4, LAMP_DROP_TARGET_5};
static const uint8_t DropTargetSwitchArray[] = {SW_DROP_TARGET_1, SW_DROP_TARGET_2, SW_DROP_TARGET_3, SW_DROP_TARGET_4, SW_DROP_TARGET_5};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
Trident2020Game::Trident2020Game() {}

// ===========================================================================
// Public interface
// ===========================================================================

void Trident2020Game::enter(unsigned long currentTime) {
   CurrentTime = currentTime;
   addPlayer(true);
   internalState_ = MACHINE_STATE_INIT_GAMEPLAY;
   internalStateChanged_ = true;
}

TopState Trident2020Game::update(unsigned long currentTime) {
   CurrentTime = currentTime;

   // Snapshot and consume the state-changed flag for this tick.
   bool curStateChanged = internalStateChanged_;
   internalStateChanged_ = false;
   int curState = internalState_;

   int returnState = curState;
   uint8_t bonusAtTop = Bonus;
   unsigned long scoreAtTop = CurrentPlayerCurrentScore;
   unsigned long scoreMultiplier = 1;
   if ((GameMode & 0x70) != 0) {
      scoreMultiplier = (unsigned long)countBits(GameMode & 0x70);
   }

   if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
      returnState = initGamePlay();
   } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
      returnState = initNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
   } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
      returnState = manageGameMode();
   } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
      returnState = countdownBonus(curStateChanged);
      showPlayerScores(CurrentPlayer, BallFirstSwitchHitTime == 0,
                       BallFirstSwitchHitTime > 0 && (CurrentTime - LastTimeScoreChanged) > 2000);
   } else if (curState == MACHINE_STATE_BALL_OVER) {
      CurrentScores[CurrentPlayer] = CurrentPlayerCurrentScore;
      StandupsHit[CurrentPlayer] = CurrentStandupsHit;
      if (SamePlayerShootsAgain) {
         returnState = MACHINE_STATE_INIT_NEW_BALL;
      } else {
         CurrentPlayer += 1;
         if (CurrentPlayer >= CurrentNumPlayers) {
            CurrentPlayer = 0;
            CurrentBallInPlay += 1;
         }
         CurrentPlayerCurrentScore = CurrentScores[CurrentPlayer];
         CurrentStandupsHit = StandupsHit[CurrentPlayer];
         scoreAtTop = CurrentPlayerCurrentScore;

         if (CurrentBallInPlay > ctx_->ballsPerGame) {
            checkHighScores();
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

   uint8_t switchHit;

   if (NumTiltWarnings <= ctx_->maxTiltWarnings) {
      while ((switchHit = machine_->pullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY) {
         switch (switchHit) {
         case SW_SLAM:
            break;

         case SW_TILT:
            if ((CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
               LastTiltWarningTime = CurrentTime;
               NumTiltWarnings += 1;
               if (NumTiltWarnings > ctx_->maxTiltWarnings) {
                  machine_->disableSolenoidStack();
                  machine_->setDisableFlippers(true);
                  machine_->turnOffAllLamps();
                  machine_->setLampState(LAMP_TILT, true);
               }
               machine_->playSoundEffect(SOUND_EFFECT_TILT_WARNING);
            }
            break;

         case SW_SELF_TEST_SWITCH:
            returnState = -1;   // any negative; detected as hardware-test exit
            machine_->setSelfTestChangedTime(CurrentTime);
            break;

         case SW_LEFT_INLANE:
            CurrentPlayerCurrentScore += ((unsigned long)RolloverValue) * (unsigned long)1000;
            addToBonus(1);
            machine_->playSoundEffect(SOUND_EFFECT_LEFT_INLANE);
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_RIGHT_INLANE:
            CurrentPlayerCurrentScore += 3000;
            addToBonus(3);
            machine_->playSoundEffect(SOUND_EFFECT_RIGHT_INLANE);
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_RIGHT_OUTLANE:
            CurrentPlayerCurrentScore += 500;
            machine_->playSoundEffect(SOUND_EFFECT_RIGHT_OUTLANE);
            if (NumberOfStandupClears == ctx_->standupSpecialLevel && !SpecialCollected) {
               SpecialCollected = true;
               if (ctx_->tournamentScoring) {
                  CurrentPlayerCurrentScore += (unsigned long)ctx_->specialValue;
               }
            }
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_10_PTS:
            CurrentPlayerCurrentScore += 10;
            machine_->playSoundEffect(SOUND_EFFECT_10PT_SWITCH);
            break;

         case SW_LEFT_SPINNER:
            if (GameMode == GAME_MODE_SKILL_SHOT) {
               CurrentPlayerCurrentScore += 10000;
               machine_->playSoundEffect(SOUND_EFFECT_LEFT_SPINNER);
            } else if ((GameMode & GAME_MODE_FEEDING_FRENZY_FLAG) != 0) {
               CurrentPlayerCurrentScore += (unsigned long)5000 * (unsigned long)scoreMultiplier;
               machine_->playSoundEffect(SOUND_EFFECT_FEEDING_FRENZY);
               if (CurrentFeedingFrenzy < 255) {
                  CurrentFeedingFrenzy += 1;
               }
            } else {
               unsigned long scoreAddition = 0;
               if ((LastStandupTargetHit & STANDUP_AMBER_MASK) != 0) scoreAddition += 400;
               if ((LastStandupTargetHit & STANDUP_WHITE_MASK) != 0) scoreAddition += 400;
               if ((LastStandupTargetHit & STANDUP_PURPLE_MASK) != 0) scoreAddition += 1000;
               if ((CurrentStandupsHit & STANDUP_AMBER_MASK) != 0) scoreAddition += 400;
               if ((CurrentStandupsHit & STANDUP_WHITE_MASK) != 0) scoreAddition += 400;
               if ((CurrentStandupsHit & STANDUP_PURPLE_MASK) != 0) scoreAddition += 1000;
               CurrentPlayerCurrentScore += (200 + (unsigned long)scoreAddition);
               if (LastSpinnerHitTime != 0 && LastSpinnerSide == 2) {
                  AlternatingSpinnerCount += 1;
               }
               LastSpinnerHitTime = CurrentTime;
               LastSpinnerSide = 1;
               machine_->playSoundEffect(SOUND_EFFECT_LEFT_SPINNER);
            }
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_RIGHT_SPINNER:
            if ((GameMode & GAME_MODE_FEEDING_FRENZY_FLAG) != 0) {
               CurrentPlayerCurrentScore += (unsigned long)5000 * (unsigned long)scoreMultiplier;
               machine_->playSoundEffect(SOUND_EFFECT_FEEDING_FRENZY);
               if (CurrentFeedingFrenzy < 255) {
                  CurrentFeedingFrenzy += 1;
               }
            } else if (GameMode != GAME_MODE_SKILL_SHOT) {
               unsigned long scoreAddition = 0;
               if ((LastStandupTargetHit & STANDUP_YELLOW_MASK) != 0) scoreAddition += 400;
               if ((LastStandupTargetHit & STANDUP_GREEN_MASK) != 0) scoreAddition += 400;
               if ((LastStandupTargetHit & STANDUP_PURPLE_MASK) != 0) scoreAddition += 1000;
               if ((CurrentStandupsHit & STANDUP_YELLOW_MASK) != 0) scoreAddition += 400;
               if ((CurrentStandupsHit & STANDUP_GREEN_MASK) != 0) scoreAddition += 400;
               if ((CurrentStandupsHit & STANDUP_PURPLE_MASK) != 0) scoreAddition += 1000;
               CurrentPlayerCurrentScore += (200 + (unsigned long)scoreAddition);
               machine_->playSoundEffect(SOUND_EFFECT_RIGHT_SPINNER);
               if (BallFirstSwitchHitTime == 0) {
                  BallFirstSwitchHitTime = CurrentTime;
               }
               if (LastSpinnerHitTime != 0 && LastSpinnerSide == 1) {
                  AlternatingSpinnerCount += 1;
               }
               LastSpinnerHitTime = CurrentTime;
               LastSpinnerSide = 2;
            }
            break;

         case SW_SAUCER:
            if (SaucerHitTime == 0 || (CurrentTime - SaucerHitTime) > 500) {
               SaucerHitTime = CurrentTime;
               ShowSaucerHit = SaucerValue;

               if (JackpotLit) {
                  FeedingFrenzySpins[CurrentPlayer] += CurrentFeedingFrenzy;
                  ExploreTheDepthsHits[CurrentPlayer] += CurrentExploreTheDepths;
                  SharpShooterHits[CurrentPlayer] += CurrentSharpShooter;
                  CurrentFeedingFrenzy = 0;
                  CurrentExploreTheDepths = 0;
                  CurrentSharpShooter = 0;
                  machine_->playSoundEffect(SOUND_EFFECT_JACKPOT);
                  CurrentPlayerCurrentScore += (unsigned long)FeedingFrenzySpins[CurrentPlayer] * 1000;
                  CurrentPlayerCurrentScore += (unsigned long)ExploreTheDepthsHits[CurrentPlayer] * 10000;
                  CurrentPlayerCurrentScore += (unsigned long)SharpShooterHits[CurrentPlayer] * 10000;
                  JackpotLit = false;
               } else {
                  CurrentPlayerCurrentScore += 1000 * ((unsigned long)SaucerValue);
                  switch (SaucerValue) {
                  case 5:  machine_->playSoundEffect(SOUND_EFFECT_SAUCER_HIT_5K);  break;
                  case 10: machine_->playSoundEffect(SOUND_EFFECT_SAUCER_HIT_10K); break;
                  case 20: machine_->playSoundEffect(SOUND_EFFECT_SAUCER_HIT_20K); break;
                  case 30: machine_->playSoundEffect(SOUND_EFFECT_SAUCER_HIT_30K); break;
                  }
               }

               if (GameMode != GAME_MODE_SKILL_SHOT) {
                  NextSaucerReduction = CurrentTime + SAUCER_DISPLAY_DURATION + 10000;
                  if (SaucerValue == 5) {
                     SaucerValue = 10;
                  } else if (SaucerValue < 30) {
                     SaucerValue += 10;
                  }
               }
               if (GameMode == GAME_MODE_MINI_GAME_QUALIFIED) {
                  GameMode = GAME_MODE_MINI_GAME_ENGAGED | GameModeFlagsQualified;
                  GameModeFlagsQualified = 0;
                  GameModeStartTime = 0;
                  machine_->pushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + MODE_START_DISPLAY_DURATION);
               } else {
                  machine_->pushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + SAUCER_DISPLAY_DURATION);
               }
            }
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_ROLLOVER:
            if (GameMode == GAME_MODE_SKILL_SHOT) {
               CurrentPlayerCurrentScore += 8000;
               RolloverValue = 6;
               machine_->playSoundEffect(SOUND_EFFECT_ROLLOVER_SKILL_SHOT);
            } else {
               CurrentPlayerCurrentScore += 1000 * ((unsigned long)RolloverValue);
               machine_->playSoundEffect(SOUND_EFFECT_ROLLOVER);
               RolloverValue += 2;
               if (RolloverValue > 14) {
                  RolloverValue = 14;
               }
            }
            RolloverFlashEndTime = CurrentTime + ROLLOVER_FLASH_DURATION;
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_OUTHOLE:
            break;

         case SW_DROP_TARGET_1:
         case SW_DROP_TARGET_2:
         case SW_DROP_TARGET_3:
         case SW_DROP_TARGET_4:
         case SW_DROP_TARGET_5:
            if (GameMode != GAME_MODE_SKILL_SHOT || (CurrentTime - GameModeStartTime) > 500) {
               handleDropTargetHit(switchHit, scoreMultiplier);
               if (BallFirstSwitchHitTime == 0) {
                  BallFirstSwitchHitTime = CurrentTime;
               }
            }
            break;

         case SW_TOP_BUMPER:
            CurrentPlayerCurrentScore += (unsigned long)100 * (unsigned long)scoreMultiplier;
            machine_->playSoundEffect(SOUND_EFFECT_TOP_BUMPER_HIT);
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_BOTTOM_BUMPER:
            CurrentPlayerCurrentScore += (unsigned long)100 * (unsigned long)scoreMultiplier;
            machine_->playSoundEffect(SOUND_EFFECT_BOTTOM_BUMPER_HIT);
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_WHITE:
         case SW_GREEN:
         case SW_AMBER:
         case SW_YELLOW:
         case SW_PURPLE:
            handleStandupHit(switchHit, scoreMultiplier);
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_UL_SLING:
         case SW_UR_SLING:
            CurrentPlayerCurrentScore += 10;
            addToBonus(1);
            machine_->playSoundEffect(SOUND_EFFECT_UPPER_SLING);
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_LL_SLING:
         case SW_LR_SLING:
            CurrentPlayerCurrentScore += 10;
            machine_->playSoundEffect(SOUND_EFFECT_LOWER_SLING);
            if (BallFirstSwitchHitTime == 0) {
               BallFirstSwitchHitTime = CurrentTime;
            }
            break;

         case SW_COIN_1:
         case SW_COIN_2:
         case SW_COIN_3:
            machine_->addCoinToAudit(switchHit);
            machine_->addCredit(true, 1);
            break;

         case SW_CREDIT_RESET:
            if (CurrentBallInPlay < 2) {
               addPlayer(false);
            } else {
               if (ctx_->credits >= 1 || ctx_->freePlayMode) {
                  if (!ctx_->freePlayMode) {
                     ctx_->credits -= 1;
                     machine_->writeByteToEEProm(RPU_CREDITS_EEPROM_BYTE, ctx_->credits);
                     machine_->setDisplayCredits(ctx_->credits);
                  }
                  returnState = MACHINE_STATE_INIT_GAMEPLAY;
               }
            }
            DEBUG_MESSAGE("Start game button pressed\n\r");
            break;
         }
      }
   } else {
      // Tilted — only service outhole and coin/test switches
      while ((switchHit = machine_->pullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY) {
         switch (switchHit) {
         case SW_SELF_TEST_SWITCH:
            returnState = -1;   // any negative; detected as hardware-test exit
            machine_->setSelfTestChangedTime(CurrentTime);
            break;
         case SW_SAUCER:
            machine_->pushToSolenoidStack(SOL_SAUCER, 5, true);
            break;
         case SW_COIN_1:
         case SW_COIN_2:
         case SW_COIN_3:
            machine_->addCoinToAudit(switchHit);
            machine_->addCredit(true, 1);
            break;
         case SW_OUTHOLE:
            if (NumTiltWarnings > ctx_->maxTiltWarnings) {
               returnState = MACHINE_STATE_COUNTDOWN_BONUS;
            }
            break;
         }
      }
   }

   if (bonusAtTop != Bonus) {
      showBonusOnTree(Bonus);
   }

   if (!ctx_->scrollingScores && CurrentPlayerCurrentScore > RPU_OS_MAX_DISPLAY_SCORE) {
      CurrentPlayerCurrentScore -= RPU_OS_MAX_DISPLAY_SCORE;
   }

   if (scoreAtTop != CurrentPlayerCurrentScore) {
      LastTimeScoreChanged = CurrentTime;
      if (!ctx_->tournamentScoring) {
         for (int awardCount = 0; awardCount < 3; awardCount++) {
            if (ctx_->awardScores[awardCount] != 0 && scoreAtTop < ctx_->awardScores[awardCount] &&
                CurrentPlayerCurrentScore >= ctx_->awardScores[awardCount]) {
               if (((ctx_->scoreAwardReplay) >> awardCount) & 0x01) {
                  machine_->addSpecialCredit();
               } else if (!ExtraBallCollected) {
                  ExtraBallCollected = true;
                  SamePlayerShootsAgain = true;
                  machine_->setLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
                  machine_->playSoundEffect(SOUND_EFFECT_EXTRA_BALL);
               }
            }
         }
      }
   }

   // Propagate internal state changes and map to the top-level state.
   if (returnState < 0) {
      return TopState::HardwareTest;   // self-test button hit during play
   }
   if (returnState == MACHINE_STATE_ATTRACT) {
      return TopState::Attract;    // game over
   }
   if (returnState != internalState_) {
      internalState_ = returnState;
      internalStateChanged_ = true;
   }
   return TopState::Game;
}

bool Trident2020Game::addPlayer(bool resetNumPlayers) {
   if (ctx_->credits < 1 && !ctx_->freePlayMode) {
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

   if (!ctx_->freePlayMode) {
      ctx_->credits -= 1;
      machine_->writeByteToEEProm(RPU_CREDITS_EEPROM_BYTE, ctx_->credits);
      machine_->setDisplayCredits(ctx_->credits);
      machine_->setCoinLockout(false);
   }
   machine_->playSoundEffect(SOUND_EFFECT_ADD_PLAYER_1 + (CurrentNumPlayers - 1));
   setPlayerLamps(CurrentNumPlayers);

   machine_->writeULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, machine_->readULFromEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

   return true;
}

// ===========================================================================
// Public lamp / display methods (called from attract mode and gameplay)
// ===========================================================================

void Trident2020Game::setPlayerLamps(uint8_t numPlayers, uint8_t playerOffset, int flashPeriod) {
   for (uint8_t i = 0; i < 4; i++) {
      machine_->setLampState(LAMP_PLAYER_1 + playerOffset + i,
                             (numPlayers == (i + 1)) ? 1 : 0, 0, (uint16_t)flashPeriod);
   }
}

void Trident2020Game::showBonusOnTree(uint8_t bonus, uint8_t dim) {
   if (bonus > MAX_DISPLAY_BONUS) {
      bonus = MAX_DISPLAY_BONUS;
   }

   uint8_t cap = 10;

   for (uint8_t turnOff = (bonus + 1); turnOff < 11; turnOff++) {
      machine_->setLampState(LAMP_BONUS_1 + (turnOff - 1), false);
   }
   if (bonus == 0) {
      return;
   }

   if (bonus >= cap) {
      while (bonus >= cap) {
         machine_->setLampState(LAMP_BONUS_1 + (cap - 1), true, dim, 250);
         bonus -= cap;
         cap -= 1;
         if (cap == 0) {
            bonus = 0;
            break;
         }
      }
      for (uint8_t turnOff = (bonus + 1); turnOff < (cap + 1); turnOff++) {
         machine_->setLampState(LAMP_BONUS_1 + (turnOff - 1), false);
      }
   }

   uint8_t bottom;
   for (bottom = 1; bottom < bonus; bottom++) {
      machine_->setLampState(LAMP_BONUS_1 + (bottom - 1), false);
   }

   if (bottom <= cap) {
      machine_->setLampState(LAMP_BONUS_1 + (bottom - 1), true, 0);
   }
}

void Trident2020Game::showSaucerLamps() {
   if (GameMode == GAME_MODE_MINI_GAME_QUALIFIED) {
      uint8_t lampPhase = ((CurrentTime - GameModeStartTime) / 100) % 4;
      for (uint8_t count = 0; count < 4; count++) {
         machine_->setLampState(LAMP_TOP_EJECT_5K - count, count == lampPhase);
      }
   } else if (SaucerHitTime != 0 && (CurrentTime - SaucerHitTime) < SAUCER_DISPLAY_DURATION) {
      uint8_t saucerLamp = 0;
      if (ShowSaucerHit > 5) {
         saucerLamp = ShowSaucerHit / 10;
      }
      for (int count = 0; count < 4; count++) {
         if (count == saucerLamp) {
            machine_->setLampState(LAMP_TOP_EJECT_5K - count, true, 0, 125);
         } else {
            machine_->setLampState(LAMP_TOP_EJECT_5K - count, false);
         }
      }
   } else if (GameMode == GAME_MODE_SKILL_SHOT) {
      uint8_t lampPhase = ((CurrentTime - GameModeStartTime) / 250) % 28;
      if (lampPhase < 16) {
         machine_->setLampState(LAMP_TOP_EJECT_5K, (lampPhase % 4) != 0, lampPhase % 2);
         SaucerValue = 5;
         for (int count = 1; count < 4; count++) {
            machine_->setLampState(LAMP_TOP_EJECT_5K - count, false);
         }
      } else {
         uint8_t saucerLamp;
         lampPhase -= 16;
         saucerLamp = lampPhase % 6;
         if (saucerLamp > 3) {
            saucerLamp = 6 - saucerLamp;
         }
         machine_->setLampState(LAMP_TOP_EJECT_5K - saucerLamp, true);
         for (int count = 0; count < 4; count++) {
            if (count != saucerLamp) {
               machine_->setLampState(LAMP_TOP_EJECT_5K - count, false);
            }
         }
      }
   } else {
      if (NextSaucerReduction != 0 && CurrentTime > NextSaucerReduction) {
         NextSaucerReduction = 0;
         if (SaucerValue > 5) {
            SaucerValue -= 10;
         }
      }
      uint8_t saucerLamp = 0;
      if (SaucerValue > 5) {
         saucerLamp = SaucerValue / 10;
      }
      for (int count = 0; count < 4; count++) {
         machine_->setLampState(LAMP_TOP_EJECT_5K - count, count == saucerLamp);
      }
   }
}

void Trident2020Game::showDropTargetLamps() {
   if (GameMode == GAME_MODE_MINI_GAME_QUALIFIED) {
      uint8_t lampPhase = 0xFF;
      if ((GameModeFlagsQualified & GAME_MODE_SHARP_SHOOTER_FLAG) != 0) {
         lampPhase = ((CurrentTime - GameModeStartTime) / 250) % 9;
      }
      for (uint8_t count = 0; count < 4; count++) {
         machine_->setLampState(LAMP_DROP_TARGET_1 - count, lampPhase == 0, 1);
         machine_->setLampState(LAMP_BONUS_2X_FEATURE - count, false);
      }
      machine_->setLampState(LAMP_DROP_TARGET_5, lampPhase == 0, 1);
   } else if ((GameMode & GAME_MODE_SHARP_SHOOTER_FLAG) != 0 ||
              (DropTargetClearTime != 0 && (CurrentTime - DropTargetClearTime) < DROP_TARGET_CLEAR_DURATION)) {
      uint8_t lampPhase = ((CurrentTime - DropTargetClearTime) / 50) % 5;
      for (uint8_t count = 0; count < 5; count++) {
         machine_->setLampState(DropTargetLampArray[count], lampPhase == count);
      }
      for (uint8_t count = 0; count < 4; count++) {
         machine_->setLampState(LAMP_BONUS_2X_FEATURE - count, false);
      }
   } else if (GameMode == GAME_MODE_SKILL_SHOT) {
      uint8_t lampPhase = ((CurrentTime - GameModeStartTime) / 110) % 8;
      for (uint8_t count = 0; count < 5; count++) {
         machine_->setLampState(DropTargetLampArray[count], (count == lampPhase) || (count == (8 - lampPhase)));
      }
      for (uint8_t count = 0; count < 4; count++) {
         machine_->setLampState(LAMP_BONUS_2X_FEATURE - count, false);
      }
   } else {
      for (uint8_t count = 0; count < 5; count++) {
         machine_->setLampState(DropTargetLampArray[count], !machine_->readSingleSwitchState(DropTargetSwitchArray[count]));
      }
      for (uint8_t count = 0; count < 4; count++) {
         machine_->setLampState(LAMP_BONUS_2X_FEATURE - count, BonusX == (count + 1));
      }
   }
}

void Trident2020Game::showStandupTargetLamps() {
   if (GameMode == GAME_MODE_MINI_GAME_QUALIFIED) {
      uint8_t lampPhase = 0xFF;
      if ((GameModeFlagsQualified & GAME_MODE_EXPLORE_THE_DEPTHS_FLAG) != 0) {
         lampPhase = ((CurrentTime - GameModeStartTime) / 250) % 9;
      }
      machine_->setLampState(LAMP_STAND_UP_PURPLE, (lampPhase == 3), 1);
      machine_->setLampState(LAMP_STAND_UP_YELLOW, (lampPhase == 3), 1);
      machine_->setLampState(LAMP_STAND_UP_AMBER,  (lampPhase == 3), 1);
      machine_->setLampState(LAMP_STAND_UP_GREEN,  (lampPhase == 3), 1);
      machine_->setLampState(LAMP_STAND_UP_WHITE,  (lampPhase == 3), 1);
   } else if ((GameMode & GAME_MODE_EXPLORE_THE_DEPTHS_FLAG) == 0 && StandupDisplayEndTime != 0 && CurrentTime < StandupDisplayEndTime) {
      machine_->setLampState(LAMP_STAND_UP_PURPLE, (CurrentStandupsHit & STANDUP_PURPLE_MASK) != 0, (LastStandupTargetHit & STANDUP_PURPLE_MASK) != 0 ? 0 : 1,
                       (LastStandupTargetHit & STANDUP_PURPLE_MASK) != 0 ? 50 : 0);
      machine_->setLampState(LAMP_STAND_UP_YELLOW, (CurrentStandupsHit & STANDUP_YELLOW_MASK) != 0, (LastStandupTargetHit & STANDUP_YELLOW_MASK) != 0 ? 0 : 1,
                       (LastStandupTargetHit & STANDUP_YELLOW_MASK) != 0 ? 50 : 0);
      machine_->setLampState(LAMP_STAND_UP_AMBER,  (CurrentStandupsHit & STANDUP_AMBER_MASK) != 0,  (LastStandupTargetHit & STANDUP_AMBER_MASK) != 0  ? 0 : 1,
                       (LastStandupTargetHit & STANDUP_AMBER_MASK) != 0  ? 50 : 0);
      machine_->setLampState(LAMP_STAND_UP_GREEN,  (CurrentStandupsHit & STANDUP_GREEN_MASK) != 0,  (LastStandupTargetHit & STANDUP_GREEN_MASK) != 0  ? 0 : 1,
                       (LastStandupTargetHit & STANDUP_GREEN_MASK) != 0  ? 50 : 0);
      machine_->setLampState(LAMP_STAND_UP_WHITE,  (CurrentStandupsHit & STANDUP_WHITE_MASK) != 0,  (LastStandupTargetHit & STANDUP_WHITE_MASK) != 0  ? 0 : 1,
                       (LastStandupTargetHit & STANDUP_WHITE_MASK) != 0  ? 50 : 0);
   } else if (GameMode == GAME_MODE_SKILL_SHOT || (GameMode & GAME_MODE_EXPLORE_THE_DEPTHS_FLAG) != 0) {
      uint8_t lampPhase = ((CurrentTime - GameModeStartTime) / 100) % 5;
      machine_->setLampState(LAMP_STAND_UP_PURPLE, lampPhase == 4 || lampPhase == 0, static_cast<uint8_t>(lampPhase == 0));
      machine_->setLampState(LAMP_STAND_UP_YELLOW, lampPhase == 3 || lampPhase == 4, static_cast<uint8_t>(lampPhase == 4));
      machine_->setLampState(LAMP_STAND_UP_AMBER,  lampPhase == 2 || lampPhase == 3, static_cast<uint8_t>(lampPhase == 3));
      machine_->setLampState(LAMP_STAND_UP_GREEN,  lampPhase == 1 || lampPhase == 2, static_cast<uint8_t>(lampPhase == 2));
      machine_->setLampState(LAMP_STAND_UP_WHITE,  lampPhase < 2,                    static_cast<uint8_t>(lampPhase == 1));
   } else {
      machine_->setLampState(LAMP_STAND_UP_PURPLE, (CurrentStandupsHit & STANDUP_PURPLE_MASK) != 0);
      machine_->setLampState(LAMP_STAND_UP_YELLOW, (CurrentStandupsHit & STANDUP_YELLOW_MASK) != 0);
      machine_->setLampState(LAMP_STAND_UP_AMBER,  (CurrentStandupsHit & STANDUP_AMBER_MASK) != 0);
      machine_->setLampState(LAMP_STAND_UP_GREEN,  (CurrentStandupsHit & STANDUP_GREEN_MASK) != 0);
      machine_->setLampState(LAMP_STAND_UP_WHITE,  (CurrentStandupsHit & STANDUP_WHITE_MASK) != 0);
   }
}

void Trident2020Game::showLeftSpinnerLamps() {
   if (GameMode == GAME_MODE_MINI_GAME_QUALIFIED) {
      uint8_t lampPhase = 0xFF;
      if ((GameModeFlagsQualified & GAME_MODE_FEEDING_FRENZY_FLAG) != 0) {
         lampPhase = ((CurrentTime - GameModeStartTime) / 250) % 9;
      }
      machine_->setLampState(LAMP_LEFT_SPINNER_AMBER,  (lampPhase == 6), 1);
      machine_->setLampState(LAMP_LEFT_SPINNER_WHITE,  (lampPhase == 6), 1);
      machine_->setLampState(LAMP_LEFT_SPINNER_PURPLE, (lampPhase == 6), 1);
   } else if (GameMode == GAME_MODE_SKILL_SHOT) {
      uint8_t lampPhase = ((CurrentTime - GameModeStartTime) / 600) % 3;
      machine_->setLampState(LAMP_LEFT_SPINNER_AMBER,  lampPhase == 0);
      machine_->setLampState(LAMP_LEFT_SPINNER_WHITE,  lampPhase == 1);
      machine_->setLampState(LAMP_LEFT_SPINNER_PURPLE, lampPhase == 2);
   } else {
      if ((GameMode & GAME_MODE_FEEDING_FRENZY_FLAG) != 0 ||
          (LastSpinnerHitTime != 0 && LastSpinnerSide == 2 && (CurrentTime - LastSpinnerHitTime) < MODE_QUALIFY_TIME)) {
         uint8_t lampPhase = ((CurrentTime - GameModeStartTime) / 100) % 3;
         machine_->setLampState(LAMP_LEFT_SPINNER_AMBER,  lampPhase == 2);
         machine_->setLampState(LAMP_LEFT_SPINNER_WHITE,  lampPhase == 1);
         machine_->setLampState(LAMP_LEFT_SPINNER_PURPLE, lampPhase == 0);
      } else {
         int flashFrequency = 200;
         if ((StandupDisplayEndTime - CurrentTime) < 1000) {
            flashFrequency = 100;
         }
         machine_->setLampState(LAMP_LEFT_SPINNER_AMBER,  (CurrentStandupsHit & STANDUP_AMBER_MASK) != 0, 0,
                          (LastStandupTargetHit & STANDUP_AMBER_MASK) != 0 ? flashFrequency : 0);
         machine_->setLampState(LAMP_LEFT_SPINNER_WHITE,  (CurrentStandupsHit & STANDUP_WHITE_MASK) != 0, 0,
                          (LastStandupTargetHit & STANDUP_WHITE_MASK) != 0 ? flashFrequency : 0);
         machine_->setLampState(LAMP_LEFT_SPINNER_PURPLE, (CurrentStandupsHit & STANDUP_PURPLE_MASK) != 0, 0,
                          (LastStandupTargetHit & STANDUP_PURPLE_MASK) != 0 ? flashFrequency : 0);
      }
   }
}

void Trident2020Game::showLeftLaneLamps() {
   uint8_t valueToShow = RolloverValue;
   int valueFlash = 0;
   if (GameMode == GAME_MODE_MINI_GAME_QUALIFIED) {
      valueToShow = 0;
   } else if (GameMode == GAME_MODE_SKILL_SHOT) {
      valueToShow = 8;
      valueFlash = 500;
   } else {
      if (RolloverFlashEndTime != 0 && CurrentTime < RolloverFlashEndTime) {
         valueFlash = 100;
      }
   }

   machine_->setLampState(LAMP_LEFT_LANE_2K, (valueToShow == 2  || valueToShow == 10), 0, valueFlash);
   machine_->setLampState(LAMP_LEFT_LANE_4K, (valueToShow == 4  || valueToShow == 12), 0, valueFlash);
   machine_->setLampState(LAMP_LEFT_LANE_6K, (valueToShow == 6  || valueToShow == 14), 0, valueFlash);
   machine_->setLampState(LAMP_LEFT_LANE_8K, (valueToShow > 6),                        0, valueFlash);
}

// ===========================================================================
// Display helpers
// ===========================================================================

void Trident2020Game::overrideScoreDisplay(uint8_t displayNum, unsigned long value, bool animate) {
   if (displayNum > 3) {
      return;
   }
   ScoreOverrideStatus |= (0x10 << displayNum);
   if (animate) {
      ScoreOverrideStatus |= (0x01 << displayNum);
   } else {
      ScoreOverrideStatus &= ~(0x01 << displayNum);
   }
   ScoreOverrideValue[displayNum] = value;
}

static uint8_t scoreDigitCount(unsigned long score) {
   if (score == 0) return 0;
   uint8_t n = 0;
   while (score > 0) { score /= 10; n++; }
   return n;
}

static uint8_t scoreDisplayMask(uint8_t numDigits) {
   uint8_t mask = 0;
   for (uint8_t i = 0; i < numDigits; i++) mask |= (0x20 >> i);
   return mask;
}

void Trident2020Game::showPlayerScores(uint8_t displayToUpdate, bool flashCurrent, bool dashCurrent,
                                       unsigned long allScoresShowValue) {
   if (displayToUpdate == 0xFF) ScoreOverrideStatus = 0;

   unsigned long overrideAnimSeed = CurrentTime / 250;
   bool          animUpdated      = false;

   for (uint8_t n = 0; n < 4; n++) {
      // Override display: always render regardless of displayToUpdate
      if (allScoresShowValue == 0 && (ScoreOverrideStatus & (0x10 << n))) {
         unsigned long overVal   = ScoreOverrideValue[n];
         bool          animated  = (ScoreOverrideStatus & (0x01 << n)) != 0;
         if (animated && overrideAnimSeed != lastTimeOverrideAnimated_) {
            uint8_t numDigits = scoreDigitCount(overVal);
            if (numDigits == 0) numDigits = 1;
            if (numDigits < (int)RPU_OS_NUM_DIGITS - 1) {
               uint8_t range = ((RPU_OS_NUM_DIGITS + 1) - numDigits) + ((RPU_OS_NUM_DIGITS - 1) - numDigits);
               uint8_t shift = (uint8_t)(overrideAnimSeed % range);
               if (shift >= ((RPU_OS_NUM_DIGITS + 1) - numDigits)) {
                  shift = (uint8_t)((RPU_OS_NUM_DIGITS - numDigits) * 2 - shift);
               }
               unsigned long shifted = overVal;
               uint8_t       mask    = scoreDisplayMask(numDigits);
               for (uint8_t d = 0; d < shift; d++) { shifted *= 10; mask >>= 1; }
               machine_->setDisplayBlank(n, 0x00);
               machine_->setDisplay(n, shifted, false);
               machine_->setDisplayBlank(n, mask);
               animUpdated = true;
            } else {
               machine_->setDisplay(n, overVal, true);
            }
         } else if (!animated) {
            machine_->setDisplay(n, overVal, true);
         }
         continue;
      }

      // For partial updates, skip non-target slots
      if (displayToUpdate != 0xFF && displayToUpdate != n) continue;

      // Blank unused player slots during full updates
      if (displayToUpdate == 0xFF && allScoresShowValue == 0 && n >= CurrentNumPlayers) {
         machine_->setDisplayBlank(n, 0x00);
         continue;
      }

      unsigned long score = allScoresShowValue ? allScoresShowValue :
                            (n == CurrentPlayer ? CurrentPlayerCurrentScore : CurrentScores[n]);

      if (flashCurrent || dashCurrent) {
         machine_->setDisplayFlash(n, score, 500);
      } else {
         machine_->setDisplay(n, score, true, 2);
      }
   }

   if (animUpdated) lastTimeOverrideAnimated_ = overrideAnimSeed;
}

// ===========================================================================
// Private lamp helpers
// ===========================================================================

void Trident2020Game::showBonusLamps() {
   if (GameMode == GAME_MODE_MINI_GAME_QUALIFIED) {
      uint8_t lightPhase = ((CurrentTime - GameModeStartTime) / 100) % 15;
      for (uint8_t count = 0; count < 10; count++) {
         machine_->setLampState(LAMP_BONUS_1 + count, (lightPhase == count) || ((lightPhase - 1) == count),
                          static_cast<uint8_t>((lightPhase - 1) == count));
      }
   } else if (Bonus != LastBonusShown) {
      LastBonusShown = Bonus;
      showBonusOnTree(Bonus);
   }
}

void Trident2020Game::showBonusXLamps() {
   if (GameMode == GAME_MODE_MINI_GAME_QUALIFIED || (GameMode & GAME_MODE_SHARP_SHOOTER_FLAG) != 0) {
      for (int count = 2; count < 6; count++) {
         machine_->setLampState(LAMP_BONUS_2X - (count - 2), false);
      }
   } else {
      for (int count = 2; count < 6; count++) {
         machine_->setLampState(LAMP_BONUS_2X - (count - 2), (count == BonusX));
      }
   }
}

void Trident2020Game::showRightSpinnerLamps() {
   if (GameMode == GAME_MODE_MINI_GAME_QUALIFIED || GameMode == GAME_MODE_SKILL_SHOT) {
      uint8_t lampPhase = 0xFF;
      if ((GameModeFlagsQualified & GAME_MODE_FEEDING_FRENZY_FLAG) != 0) {
         lampPhase = ((CurrentTime - GameModeStartTime) / 250) % 9;
      }
      machine_->setLampState(LAMP_RIGHT_SPINNER_YELLOW, (lampPhase == 6), 1);
      machine_->setLampState(LAMP_RIGHT_SPINNER_GREEN,  (lampPhase == 6), 1);
      machine_->setLampState(LAMP_RIGHT_SPINNER_PURPLE, (lampPhase == 6), 1);
   } else {
      if ((GameMode & GAME_MODE_FEEDING_FRENZY_FLAG) != 0 ||
          (LastSpinnerHitTime != 0 && LastSpinnerSide == 1 && (CurrentTime - LastSpinnerHitTime) < MODE_QUALIFY_TIME)) {
         uint8_t lampPhase = ((CurrentTime - GameModeStartTime) / 100) % 3;
         machine_->setLampState(LAMP_RIGHT_SPINNER_YELLOW, lampPhase == 2);
         machine_->setLampState(LAMP_RIGHT_SPINNER_GREEN,  lampPhase == 1);
         machine_->setLampState(LAMP_RIGHT_SPINNER_PURPLE, lampPhase == 0);
      } else {
         int flashFrequency = 200;
         if ((StandupDisplayEndTime - CurrentTime) < 1000) {
            flashFrequency = 100;
         }
         machine_->setLampState(LAMP_RIGHT_SPINNER_YELLOW, (CurrentStandupsHit & STANDUP_YELLOW_MASK) != 0, 0,
                          (LastStandupTargetHit & STANDUP_YELLOW_MASK) != 0 ? flashFrequency : 0);
         machine_->setLampState(LAMP_RIGHT_SPINNER_GREEN,  (CurrentStandupsHit & STANDUP_GREEN_MASK) != 0, 0,
                          (LastStandupTargetHit & STANDUP_GREEN_MASK) != 0 ? flashFrequency : 0);
         machine_->setLampState(LAMP_RIGHT_SPINNER_PURPLE, (CurrentStandupsHit & STANDUP_PURPLE_MASK) != 0, 0,
                          (LastStandupTargetHit & STANDUP_PURPLE_MASK) != 0 ? flashFrequency : 0);
      }
   }
}

void Trident2020Game::showAwardLamps() {
   machine_->setLampState(LAMP_EXTRA_BALL, ((NumberOfStandupClears == 1 && !ExtraBallCollected) || RescueFromTheDeepEndTime != 0), 0,
                    (RescueFromTheDeepEndTime != 0) ? 100 : 0);
   machine_->setLampState(LAMP_DROP_TARGET_SPECIAL,
                    (BonusX == (ctx_->targetSpecialBonus - 1)) && (GameMode & GAME_MODE_SHARP_SHOOTER_FLAG) == 0);
   machine_->setLampState(LAMP_STAND_UP_SPECIAL,
                    (NumberOfStandupClears == (ctx_->standupSpecialLevel - 1)) && (GameMode & GAME_MODE_EXPLORE_THE_DEPTHS_FLAG) == 0);
   machine_->setLampState(LAMP_RIGHT_OUTLANE_SPECIAL, (NumberOfStandupClears == ctx_->standupSpecialLevel && !SpecialCollected));
}

void Trident2020Game::showShootAgainLamp() {
   if (!BallSaveUsed && ctx_->ballSaveNumSeconds > 0 &&
       (CurrentTime - BallFirstSwitchHitTime) < ((unsigned long)(ctx_->ballSaveNumSeconds - 1) * 1000)) {
      unsigned long msRemaining = ((unsigned long)(ctx_->ballSaveNumSeconds - 1) * 1000) - (CurrentTime - BallFirstSwitchHitTime);
      machine_->setLampState(LAMP_SHOOT_AGAIN, true, 0, (msRemaining < 1000) ? 100 : 500);
   } else {
      machine_->setLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
   }
}

// ===========================================================================
// Game play helpers
// ===========================================================================

uint8_t Trident2020Game::countBits(uint8_t byteToBeCounted) {
   uint8_t numBits = 0;
   for (uint8_t count = 0; count < 8; count++) {
      numBits += (byteToBeCounted & 0x01);
      byteToBeCounted = byteToBeCounted >> 1;
   }
   return numBits;
}

void Trident2020Game::resetDropTargets() {
   machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_RESET, 12, CurrentTime + 400);
   DropTargetClearTime = CurrentTime;

   if ((GameMode & GAME_MODE_SHARP_SHOOTER_FLAG) != 0) {
      if (SharpShooterTarget != 1) machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_1, 7, CurrentTime + 600);
      if (SharpShooterTarget != 2) machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_2, 7, CurrentTime + 625);
      if (SharpShooterTarget != 3) machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_3, 7, CurrentTime + 650);
      if (SharpShooterTarget != 4) machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_4, 7, CurrentTime + 675);
      if (SharpShooterTarget != 5) machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_5, 7, CurrentTime + 700);
      CurrentDropTargetsValid = 0x01 << (SharpShooterTarget - 1);
   } else {
      if (BonusX == 1) {
         machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_1, 4, CurrentTime + 700);
         machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_3, 4, CurrentTime + 750);
         machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_5, 4, CurrentTime + 800);
         CurrentDropTargetsValid = 0x0A;
      } else if (BonusX == 2) {
         machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_2, 4, CurrentTime + 600);
         machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_4, 4, CurrentTime + 650);
         CurrentDropTargetsValid = 0x15;
      } else if (BonusX == 3) {
         machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_3, 4, CurrentTime + 600);
         CurrentDropTargetsValid = 0x1B;
      } else {
         CurrentDropTargetsValid = 0x1F;
      }
   }
}

void Trident2020Game::handleDropTargetHit(uint8_t switchHit, unsigned long scoreMultiplier) {
   if (GameMode == GAME_MODE_SKILL_SHOT) {
      BonusX = 2;
      machine_->playSoundEffect(SOUND_EFFECT_DT_SKILL_SHOT);
      resetDropTargets();
      CurrentPlayerCurrentScore += 10000;
   } else {
      uint8_t switchMask = 1 << (SW_DROP_TARGET_1 - switchHit);

      if ((switchMask & CurrentDropTargetsValid) != 0) {
         if (machine_->readSingleSwitchState(SW_DROP_TARGET_1) && machine_->readSingleSwitchState(SW_DROP_TARGET_2) &&
             machine_->readSingleSwitchState(SW_DROP_TARGET_3) && machine_->readSingleSwitchState(SW_DROP_TARGET_4) &&
             machine_->readSingleSwitchState(SW_DROP_TARGET_5)) {
            if ((GameMode & GAME_MODE_SHARP_SHOOTER_FLAG) == 0) {
               BonusX += 1;
               machine_->playSoundEffect(SOUND_EFFECT_DROP_TARGET_CLEAR_1 + (BonusX - 1));
               if (BonusX == ctx_->targetSpecialBonus) {
                  if (ctx_->tournamentScoring) {
                     CurrentPlayerCurrentScore += ctx_->specialValue;
                  } else {
                     machine_->addSpecialCredit();
                  }
               }

               if (BonusX == ctx_->sharpShooterStartBonus && !(GameModeFlagsQualified & GAME_MODE_SHARP_SHOOTER_FLAG)) {
                  GameModeFlagsQualified |= GAME_MODE_SHARP_SHOOTER_FLAG;
                  machine_->playSoundEffect(SOUND_EFFECT_SHARP_SHOOTER_QUALIFIED);
                  SharpShooterTarget = 1;
                  if ((GameMode & 0x0F) == GAME_MODE_MINI_GAME_QUALIFIED) {
                     GameModeEndTime = CurrentTime + MODE_QUALIFY_TIME;
                  }
               }
            } else {
               if (CurrentSharpShooter < 255) {
                  CurrentSharpShooter += 1;
               }
               CurrentPlayerCurrentScore += (unsigned long)2000 * (unsigned long)scoreMultiplier;
               SharpShooterTarget += 1;
               if (SharpShooterTarget > 5) {
                  SharpShooterTarget = 1;
               }
               machine_->playSoundEffect(SOUND_EFFECT_SHARP_SHOOTER_HIT);
            }

            if (BonusX > 5) {
               BonusX = 5;
            }
            CurrentPlayerCurrentScore += ((unsigned long)1000 * (unsigned long)BonusX);
            resetDropTargets();
         } else {
            CurrentPlayerCurrentScore += 500;
            machine_->playSoundEffect(SOUND_EFFECT_DROP_TARGET);
         }
      }
   }
}

void Trident2020Game::handleStandupHit(uint8_t switchHit, unsigned long scoreMultiplier) {
   uint8_t switchMask = (1 << (switchHit - 19));

   if ((GameMode & GAME_MODE_EXPLORE_THE_DEPTHS_FLAG) == 0) {
      if (CurrentTime > StandupDisplayEndTime || StandupDisplayEndTime == 0) {
         StandupDisplayEndTime = CurrentTime + STANDUP_HIT_DISPLAY_DURATION;
         LastStandupTargetHit = 0;
      } else {
         uint8_t numSwitchesOn = countBits(LastStandupTargetHit);
         if (numSwitchesOn > 3) {
            numSwitchesOn = 3;
         }
         StandupDisplayEndTime = CurrentTime + STANDUP_HIT_DISPLAY_DURATION * numSwitchesOn;
      }

      if (GameMode == GAME_MODE_SKILL_SHOT) {
         CurrentPlayerCurrentScore += 15000;
         machine_->playSoundEffect(SOUND_EFFECT_SU_SKILL_SHOT);
      } else {
         uint8_t numSwitchesOn = countBits(switchMask | CurrentStandupsHit);
         machine_->playSoundEffect(SOUND_EFFECT_FIRST_SU_SWITCH_HIT + (numSwitchesOn - 1));
         if ((CurrentStandupsHit & switchMask) != 0) {
            CurrentPlayerCurrentScore += 500;
         } else {
            CurrentPlayerCurrentScore += 1000;
         }
      }
      CurrentStandupsHit |= switchMask;
      LastStandupTargetHit |= switchMask;
   } else {
      machine_->playSoundEffect(SOUND_EFFECT_EXPLORE_HIT);
      CurrentPlayerCurrentScore += (unsigned long)10000 * (unsigned long)scoreMultiplier;
      if (CurrentExploreTheDepths < 255) {
         CurrentExploreTheDepths += 1;
      }
   }

   if (CurrentStandupsHit == 0x1F) {
      CurrentStandupsHit = 0;
      LastStandupTargetHit = 0;
      NumberOfStandupClears += 1;
      if (NumberOfStandupClears == ctx_->standupSpecialLevel) {
         if (ctx_->tournamentScoring) {
            CurrentPlayerCurrentScore += (unsigned long)ctx_->specialValue;
         } else {
            machine_->addSpecialCredit();
         }
      }
      if ((NumberOfStandupClears % ExploreTheDepthsStart) == 0 && !(GameModeFlagsQualified & GAME_MODE_EXPLORE_THE_DEPTHS_FLAG)) {
         machine_->playSoundEffect(SOUND_EFFECT_EXPLORE_QUALIFIED);
         GameModeFlagsQualified |= GAME_MODE_EXPLORE_THE_DEPTHS_FLAG;
         if ((GameMode & 0x0F) == GAME_MODE_MINI_GAME_QUALIFIED) {
            GameModeEndTime = CurrentTime + MODE_QUALIFY_TIME;
         }
      } else {
         machine_->playSoundEffect(SOUND_EFFECT_STANDUPS_CLEARED);
      }
   }
}

// ===========================================================================
// Game phase methods
// ===========================================================================

int Trident2020Game::initGamePlay() {
   DEBUG_MESSAGE("Starting game\n\r");

   machine_->enableSolenoidStack();
   machine_->setCoinLockout(ctx_->credits >= ctx_->maximumCredits);
   machine_->turnOffAllLamps();
   setPlayerLamps(1);

   ctx_->resetScoresToClearVersion = false;

   for (int count = 0; count < 4; count++) {
      machine_->setDisplay(count, 0);
      if (count == 0) {
         machine_->setDisplayBlank(count, 0x30);
      } else {
         machine_->setDisplayBlank(count, 0x00);
      }
      CurrentScores[count] = 0;
      SamePlayerShootsAgain = false;
      StandupsHit[count] = 0;
      FeedingFrenzySpins[count] = 0;
      ExploreTheDepthsHits[count] = 0;
      SharpShooterHits[count] = 0;
   }

   CurrentBallInPlay = 1;
   CurrentNumPlayers = 1;
   CurrentPlayer = 0;
   showPlayerScores(0xFF, false, false);

   if (machine_->readSingleSwitchState(SW_SAUCER)) {
      machine_->pushToSolenoidStack(SOL_SAUCER, 5);
   }

   return MACHINE_STATE_INIT_NEW_BALL;
}

int Trident2020Game::initNewBall(bool curStateChanged, uint8_t playerNum, int ballNum) {
   if (curStateChanged) {
      SamePlayerShootsAgain = false;
      BallFirstSwitchHitTime = 0;

      machine_->setDisableFlippers(false);
      machine_->enableSolenoidStack();
      machine_->setDisplayCredits(ctx_->credits, true);
      setPlayerLamps(playerNum + 1, 4);
      if (CurrentNumPlayers > 1 && (ballNum != 1 || playerNum != 0)) {
         machine_->playSoundEffect(SOUND_EFFECT_PLAYER_1_UP + playerNum);
      }
      machine_->playBackgroundSongBasedOnBall(ballNum);

      machine_->setDisplayBallInPlay(ballNum);
      machine_->setLampState(LAMP_BALL_IN_PLAY, true);
      machine_->setLampState(LAMP_TILT, false);

      if (ctx_->ballSaveNumSeconds > 0) {
         machine_->setLampState(LAMP_SHOOT_AGAIN, true, 0, 500);
      }

      Bonus = 1;
      showBonusOnTree(1);
      BonusX = 1;
      BallSaveUsed = false;
      BallTimeInTrough = 0;
      NumTiltWarnings = 0;
      LastTiltWarningTime = 0;

      GameModeStartTime = CurrentTime;
      GameMode = GAME_MODE_SKILL_SHOT;
      GameModeFlagsQualified = 0;
      SaucerValue = 5;
      NextSaucerReduction = 0;
      ShowSaucerHit = 0;
      SaucerHitTime = 0;
      DropTargetClearTime = 0;
      StandupDisplayEndTime = 0;
      CurrentDropTargetsValid = 0x1F;
      RolloverValue = 2;
      RolloverFlashEndTime = 0;
      RescueFromTheDeepEndTime = 0;
      RescueFromTheDeepAvailable = true;
      LastSpinnerSide = 0;
      AlternatingSpinnerCount = 0;
      LastSpinnerHitTime = 0;
      NumberOfStandupClears = 0;
      CurrentFeedingFrenzy = 0;
      CurrentExploreTheDepths = 0;
      CurrentSharpShooter = 0;
      ExtraBallCollected = false;
      ShowingModeStats = false;
      JackpotLit = false;

      CurrentPlayerCurrentScore = CurrentScores[CurrentPlayer];
      CurrentStandupsHit = StandupsHit[CurrentPlayer];

      if (machine_->readSingleSwitchState(SW_OUTHOLE)) {
         machine_->pushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
      }

      machine_->pushToTimedSolenoidStack(SOL_DROP_TARGET_RESET, 12, CurrentTime);
   }

   if (machine_->readSingleSwitchState(SW_OUTHOLE)) {
      return MACHINE_STATE_INIT_NEW_BALL;
   } else {
      return MACHINE_STATE_NORMAL_GAMEPLAY;
   }
}

void Trident2020Game::addToBonus(uint8_t bonusAddition) {
   Bonus += bonusAddition;
   if (Bonus > MAX_DISPLAY_BONUS) {
      Bonus = MAX_DISPLAY_BONUS;
   }
}

void Trident2020Game::checkForFeedingFrenzyQualify() {
   if (AlternatingSpinnerCount == 0) {
      return;
   }

   if (LastSpinnerHitTime != 0 && (CurrentTime - LastSpinnerHitTime) > FEEDING_FRENZY_ALTERNATE_TIME) {
      LastSpinnerHitTime = 0;
      AlternatingSpinnerCount = 0;
      LastSpinnerSide = 0;
   }
   if (AlternatingSpinnerCount == 3 && !(GameModeFlagsQualified & GAME_MODE_FEEDING_FRENZY_FLAG)) {
      GameModeFlagsQualified |= GAME_MODE_FEEDING_FRENZY_FLAG;
      machine_->playSoundEffect(SOUND_EFFECT_FEEDING_FRENZY_QUALIFIED);
      if ((GameMode & 0x0F) == GAME_MODE_MINI_GAME_QUALIFIED) {
         GameModeEndTime = CurrentTime + MODE_QUALIFY_TIME;
      }
   }
}

int Trident2020Game::manageGameMode() {
   int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

   if (BallFirstSwitchHitTime == 0) {
      if (!PlayerUpLightBlinking) {
         setPlayerLamps((CurrentPlayer + 1), 4, 250);
         PlayerUpLightBlinking = true;
      }
   } else {
      if (PlayerUpLightBlinking) {
         setPlayerLamps((CurrentPlayer + 1), 4);
         PlayerUpLightBlinking = false;
      }
   }

   if (CurrentTime > RescueFromTheDeepEndTime) {
      RescueFromTheDeepEndTime = 0;
   }

   switch ((GameMode & 0x0F)) {
   case GAME_MODE_SKILL_SHOT:
      if (BallFirstSwitchHitTime != 0) {
         GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
         GameModeStartTime = 0;
         resetDropTargets();
         DEBUG_MESSAGE("Exit skill shot - Changing to Qualify Select\n\r");
      }
      break;

   case GAME_MODE_UNSTRUCTURED_PLAY:
      if (GameModeStartTime == 0) {
         GameModeStartTime = CurrentTime;
      }

      if (CurrentTime > StandupDisplayEndTime) {
         LastStandupTargetHit = 0;
      }

      checkForFeedingFrenzyQualify();

      if ((CurrentTime - LastTimeScoreChanged) > 2000 && (((CurrentTime - LastTimeScoreChanged) / 4000) % 2) == 0) {
         if (!ShowingModeStats &&
             (FeedingFrenzySpins[CurrentPlayer] != 0 || SharpShooterHits[CurrentPlayer] != 0 || ExploreTheDepthsHits[CurrentPlayer] != 0)) {
            int modeStatShown = 0;
            for (int displayCount = 0; displayCount < 4; displayCount++) {
               if (displayCount != CurrentPlayer) {
                  if (modeStatShown == 0) overrideScoreDisplay(displayCount, FeedingFrenzySpins[CurrentPlayer], false);
                  if (modeStatShown == 1) overrideScoreDisplay(displayCount, SharpShooterHits[CurrentPlayer], false);
                  if (modeStatShown == 2) overrideScoreDisplay(displayCount, ExploreTheDepthsHits[CurrentPlayer], false);
                  modeStatShown += 1;
               }
            }
            ShowingModeStats = true;
         }
      } else {
         if (ShowingModeStats) {
            ShowingModeStats = false;
            showPlayerScores(0xFF, false, false);
         }
      }

      if (GameModeFlagsQualified != 0) {
         GameMode = GAME_MODE_MINI_GAME_QUALIFIED;
         GameModeStartTime = 0;
      }
      break;

   case GAME_MODE_MINI_GAME_QUALIFIED:
      if (GameModeStartTime == 0) {
         GameModeStartTime = CurrentTime;
         GameModeEndTime = CurrentTime + MODE_QUALIFY_TIME;
      }
      checkForFeedingFrenzyQualify();

      if (CurrentTime > GameModeEndTime) {
         GameModeStartTime = 0;
         GameModeFlagsQualified = 0;
         GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
      }
      break;

   case GAME_MODE_MINI_GAME_ENGAGED:
      if (GameModeStartTime == 0) {
         GameModeStartTime = CurrentTime;
         uint8_t modeStartSound = SOUND_EFFECT_FEEDING_FRENZY_START;
         switch ((GameMode & 0x70)) {
         case GAME_MODE_SHARP_SHOOTER_FLAG:
            modeStartSound = SOUND_EFFECT_SHARP_SHOOTER_START;
            break;
         case GAME_MODE_EXPLORE_THE_DEPTHS_FLAG:
            modeStartSound = SOUND_EFFECT_EXPLORE_THE_DEPTHS_START;
            break;
         case GAME_MODE_FEEDING_FRENZY_FLAG:
            modeStartSound = SOUND_EFFECT_FEEDING_FRENZY_START;
            break;
         case (GAME_MODE_SHARP_SHOOTER_FLAG | GAME_MODE_FEEDING_FRENZY_FLAG):
            modeStartSound = SOUND_EFFECT_SS_AND_FF_START;
            break;
         case (GAME_MODE_SHARP_SHOOTER_FLAG | GAME_MODE_EXPLORE_THE_DEPTHS_FLAG):
            modeStartSound = SOUND_EFFECT_SS_AND_ETD_START;
            break;
         case (GAME_MODE_FEEDING_FRENZY_FLAG | GAME_MODE_EXPLORE_THE_DEPTHS_FLAG):
            modeStartSound = SOUND_EFFECT_FF_AND_ETD_START;
            break;
         case (GAME_MODE_SHARP_SHOOTER_FLAG | GAME_MODE_FEEDING_FRENZY_FLAG | GAME_MODE_EXPLORE_THE_DEPTHS_FLAG):
            modeStartSound = SOUND_EFFECT_MEGA_STACK_START;
            break;
         }
         machine_->playSoundEffect(modeStartSound);

         uint8_t numMiniGames = countBits(0xF0 & GameMode);
         if (numMiniGames == 1) {
            machine_->playBackgroundSong(SOUND_EFFECT_BACKGROUND_FOR_SINGLE_MODE);
            GameModeEndTime = CurrentTime + MINI_GAME_SINGLE_DURATION;
         } else if (numMiniGames == 2) {
            machine_->playBackgroundSong(SOUND_EFFECT_BACKGROUND_FOR_DOUBLE_MODE);
            GameModeEndTime = CurrentTime + MINI_GAME_DOUBLE_DURATION;
         } else {
            machine_->playBackgroundSong(SOUND_EFFECT_BACKGROUND_FOR_TRIPLE_MODE);
            GameModeEndTime = CurrentTime + MINI_GAME_TRIPLE_DURATION;
         }
      }

      if ((CurrentTime - GameModeStartTime) > MODE_START_DISPLAY_DURATION) {
         for (uint8_t count = 0; count < 4; count++) {
            if (count != CurrentPlayer) {
               overrideScoreDisplay(count, (GameModeEndTime - CurrentTime) / 1000, true);
            }
         }
      }

      if ((GameMode & GAME_MODE_SHARP_SHOOTER_FLAG) != 0) {
         if ((CurrentTime - DropTargetClearTime) > SHARP_SHOOTER_TARGET_TIME) {
            SharpShooterTarget += 1;
            if (SharpShooterTarget > 5) {
               SharpShooterTarget = 1;
            }
            resetDropTargets();
         }
      }

      if (CurrentTime > GameModeEndTime) {
         GameModeEndTime = 0;
         GameModeStartTime = 0;
         LastMiniGameBonusTime = 0;
         showPlayerScores(0xFF, false, false);
         machine_->playBackgroundSong(SOUND_EFFECT_NONE);
         GameMode = GAME_MODE_MINI_GAME_REWARD_COUNTDOWN;
      }
      break;

   case GAME_MODE_MINI_GAME_REWARD_COUNTDOWN:
      if (GameModeStartTime == 0) {
         GameModeStartTime = CurrentTime;
      }
      if (LastMiniGameBonusTime == 0 || (CurrentTime - LastMiniGameBonusTime) > 250) {
         if (CurrentFeedingFrenzy > 0) {
            CurrentFeedingFrenzy -= 1;
            FeedingFrenzySpins[CurrentPlayer] += 1;
            CurrentPlayerCurrentScore += 1000;
            machine_->playSoundEffect(SOUND_EFFECT_FEEDING_FRENZY);
         } else if (CurrentSharpShooter > 0) {
            CurrentSharpShooter -= 1;
            SharpShooterHits[CurrentPlayer] += 1;
            CurrentPlayerCurrentScore += 2500;
            machine_->playSoundEffect(SOUND_EFFECT_SHARP_SHOOTER_HIT);
         } else if (CurrentExploreTheDepths > 0) {
            CurrentExploreTheDepths -= 1;
            ExploreTheDepthsHits[CurrentPlayer] += 1;
            CurrentPlayerCurrentScore += 2500;
            machine_->playSoundEffect(SOUND_EFFECT_EXPLORE_HIT);
         } else {
            GameModeEndTime = 0;
            GameModeStartTime = 0;
            if (FeedingFrenzySpins[CurrentPlayer] != 0 && SharpShooterHits[CurrentPlayer] != 0 && ExploreTheDepthsHits[CurrentPlayer] != 0) {
               GameMode = GAME_MODE_WIZARD;
            } else {
               GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
               machine_->playBackgroundSongBasedOnBall(CurrentBallInPlay);
               machine_->playSoundEffect(SOUND_EFFECT_MODE_FINISHED);
            }
         }
         LastMiniGameBonusTime = CurrentTime;
      }
      break;

   case GAME_MODE_WIZARD_WITHOUT_FLAGS:
      if (GameModeStartTime == 0) {
         GameModeStartTime = CurrentTime;
         GameModeEndTime = CurrentTime + WIZARD_MODE_DURATION;
         machine_->playBackgroundSong(SOUND_EFFECT_BACKGROUND_WIZ);
         machine_->playSoundEffect(SOUND_EFFECT_DEEP_BLUE_SEA_MODE);
         JackpotLit = true;
      }

      if (!JackpotLit) {
         if (CurrentFeedingFrenzy != 0 && CurrentSharpShooter != 0 && CurrentExploreTheDepths != 0) {
            JackpotLit = true;
         }
      }

      for (uint8_t count = 0; count < 4; count++) {
         if (count != CurrentPlayer) {
            overrideScoreDisplay(count, (GameModeEndTime - CurrentTime) / 1000, true);
         }
      }

      if (CurrentTime > GameModeEndTime) {
         FeedingFrenzySpins[CurrentPlayer] = 0;
         SharpShooterHits[CurrentPlayer] = 0;
         ExploreTheDepthsHits[CurrentPlayer] = 0;
         JackpotLit = false;
         GameModeEndTime = 0;
         GameModeStartTime = 0;
         LastMiniGameBonusTime = 0;
         showPlayerScores(0xFF, false, false);
         machine_->playBackgroundSongBasedOnBall(CurrentBallInPlay);
         machine_->playSoundEffect(SOUND_EFFECT_MODE_FINISHED);
         GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
      }
      break;
   }

   if (GameModeStartTime != 0) {
      showSaucerLamps();
      showDropTargetLamps();
      showStandupTargetLamps();
      showBonusLamps();
      showBonusXLamps();
      showLeftSpinnerLamps();
      showRightSpinnerLamps();
      showLeftLaneLamps();
      showAwardLamps();
      showShootAgainLamp();
      showPlayerScores(CurrentPlayer, BallFirstSwitchHitTime == 0,
                       BallFirstSwitchHitTime > 0 && (CurrentTime - LastTimeScoreChanged) > 2000);
   }

   // Check to see if ball is in the outhole
   if (machine_->readSingleSwitchState(SW_OUTHOLE)) {
      if (BallTimeInTrough == 0) {
         BallTimeInTrough = CurrentTime;
      } else {
         if ((CurrentTime - BallTimeInTrough) > 500) {
            if (BallFirstSwitchHitTime == 0 && NumTiltWarnings <= ctx_->maxTiltWarnings) {
               machine_->pushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime);
               BallTimeInTrough = 0;
               returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
            } else {
               if (!BallSaveUsed && ((CurrentTime - BallFirstSwitchHitTime) / 1000) < ((unsigned long)ctx_->ballSaveNumSeconds)) {
                  machine_->pushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
                  BallSaveUsed = true;
                  machine_->playSoundEffect(SOUND_EFFECT_SWIM_AGAIN);
                  machine_->setLampState(LAMP_SHOOT_AGAIN, false);
                  BallTimeInTrough = CurrentTime;
                  returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
               } else if (RescueFromTheDeepEndTime != 0 && CurrentTime < RescueFromTheDeepEndTime) {
                  machine_->pushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
                  machine_->playSoundEffect(SOUND_EFFECT_RESCUE_FROM_THE_DEEP);
                  RescueFromTheDeepAvailable = false;
                  BallTimeInTrough = CurrentTime;
                  returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
               } else {
                  showPlayerScores(0xFF, false, false);
                  FeedingFrenzySpins[CurrentPlayer] += CurrentFeedingFrenzy;
                  ExploreTheDepthsHits[CurrentPlayer] += CurrentExploreTheDepths;
                  SharpShooterHits[CurrentPlayer] += CurrentSharpShooter;
                  machine_->playBackgroundSong(SOUND_EFFECT_NONE);
                  returnState = MACHINE_STATE_COUNTDOWN_BONUS;
               }
            }
         }
      }
   } else {
      BallTimeInTrough = 0;
   }

   return returnState;
}

int Trident2020Game::countdownBonus(bool curStateChanged) {
   if (curStateChanged) {
      machine_->setLampState(LAMP_BALL_IN_PLAY, true, 0, 250);
      CountdownStartTime = CurrentTime;
      showBonusOnTree(Bonus);
      LastCountdownReportTime = CountdownStartTime;
      BonusCountDownEndTime = 0xFFFFFFFF;
   }

   if ((CurrentTime - LastCountdownReportTime) > 200) {
      if (Bonus > 0) {
         if (NumTiltWarnings <= ctx_->maxTiltWarnings) {
            machine_->playSoundEffect(SOUND_EFFECT_BONUS_COUNT + (BonusX - 1));
            CurrentPlayerCurrentScore += (unsigned long)1000 * ((unsigned long)BonusX);
         }
         Bonus -= 1;
         showBonusOnTree(Bonus);
      } else if (BonusCountDownEndTime == 0xFFFFFFFF) {
         machine_->playSoundEffect(SOUND_EFFECT_BALL_OVER);
         machine_->setLampState(LAMP_BONUS_1, false);
         BonusCountDownEndTime = CurrentTime + 1000;
      }
      LastCountdownReportTime = CurrentTime;
   }

   if (CurrentTime > BonusCountDownEndTime) {
      BonusCountDownEndTime = 0xFFFFFFFF;
      return MACHINE_STATE_BALL_OVER;
   }

   return MACHINE_STATE_COUNTDOWN_BONUS;
}

void Trident2020Game::checkHighScores() {
   unsigned long highestScore = 0;
   int highScorePlayerNum = 0;
   for (int count = 0; count < CurrentNumPlayers; count++) {
      if (CurrentScores[count] > highestScore) {
         highestScore = CurrentScores[count];
      }
      highScorePlayerNum = count;
   }

   if (highestScore > ctx_->highScore) {
      ctx_->highScore = highestScore;
      if (ctx_->highScoreReplay) {
         machine_->addCredit(false, 3);
         machine_->writeULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, machine_->readULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
      }
      machine_->writeULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, highestScore);
      machine_->writeULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, machine_->readULFromEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

      for (int count = 0; count < 4; count++) {
         if (count == highScorePlayerNum) {
            machine_->setDisplay(count, CurrentScores[count], true, 2);
         } else {
            machine_->setDisplayBlank(count, 0x00);
         }
      }

      machine_->pushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
      machine_->pushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 300, true);
      machine_->pushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 600, true);
   }
}

int Trident2020Game::showMatchSequence(bool curStateChanged) {
   if (!ctx_->matchFeature) {
      return MACHINE_STATE_ATTRACT;
   }

   if (curStateChanged) {
      MatchSequenceStartTime = CurrentTime;
      MatchDelay = 1500;
      MatchDigit = CurrentTime % 10;
      NumMatchSpins = 0;
      machine_->setLampState(LAMP_MATCH, true, 0);
      machine_->setDisableFlippers(true);
      ScoreMatches = 0;
      machine_->setLampState(LAMP_BALL_IN_PLAY, false);
   }

   if (NumMatchSpins < 40) {
      if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
         MatchDigit += 1;
         if (MatchDigit > 9) {
            MatchDigit = 0;
         }
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
         if ((CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
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
