/**************************************************************************
 * TridentMachine.cpp
 *
 * Concrete PinballMachine for the Trident hardware. Owns the audio
 * subsystem and MachineSettings; implements all credit / coin-mech
 * operations that were previously free functions in main.cpp.
 **************************************************************************/

#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include "TridentMachine.h"
#include <Arduino.h>
#include <EEPROM.h>

// ---------------------------------------------------------------------------
// Static data
// ---------------------------------------------------------------------------

const unsigned short TridentMachine::kChuteAuditByte[3] = {
   RPU_CHUTE_1_COINS_START_BYTE,
   RPU_CHUTE_2_COINS_START_BYTE,
   RPU_CHUTE_3_COINS_START_BYTE,
};

const uint8_t TridentMachine::kCPCPairs[9][2] = {
   {1, 5}, {1, 4}, {1, 3}, {1, 2}, {1, 1},
   {2, 3}, {2, 1}, {3, 1}, {4, 1}
};

const uint16_t TridentMachine::kCPCChuteByte[3] = {
   RPU_CPC_CHUTE_1_SELECTION_BYTE,
   RPU_CPC_CHUTE_2_SELECTION_BYTE,
   RPU_CPC_CHUTE_3_SELECTION_BYTE,
};

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void TridentMachine::ensureCPCRead() {
   if (cpcSelectionsRead_) return;
   for (uint8_t i = 0; i < 3; i++) {
      cpcSelection_[i] = RPU_ReadByteFromEEProm(kCPCChuteByte[i]);
      if (cpcSelection_[i] >= 9) {
         cpcSelection_[i] = 4;
         RPU_WriteByteToEEProm(kCPCChuteByte[i], 4);
      }
   }
   cpcSelectionsRead_ = true;
}

uint8_t TridentMachine::getCPCSelection(uint8_t chuteNum) {
   if (chuteNum > 2) return 0xFF;
   ensureCPCRead();
   return cpcSelection_[chuteNum];
}

uint8_t TridentMachine::getCPCPairCoins(uint8_t pairIndex) {
   return (pairIndex < 9) ? kCPCPairs[pairIndex][0] : 1;
}

uint8_t TridentMachine::getCPCPairCredits(uint8_t pairIndex) {
   return (pairIndex < 9) ? kCPCPairs[pairIndex][1] : 1;
}

void TridentMachine::setCPCSelection(uint8_t chuteNum, uint8_t pairIdx) {
   if (chuteNum >= 3) return;
   ensureCPCRead();
   cpcSelection_[chuteNum] = pairIdx;
   RPU_WriteByteToEEProm(kCPCChuteByte[chuteNum], pairIdx);
}

uint8_t TridentMachine::readSetting(int addr, uint8_t defaultValue) {
   uint8_t value = EEPROM.read(addr);
   if (value == 0xFF) {
      EEPROM.write(addr, defaultValue);
      return defaultValue;
   }
   return value;
}

uint8_t TridentMachine::switchToChuteNum(uint8_t switchHit) {
   if (switchHit == SW_COIN_2) return 1;
   if (switchHit == SW_COIN_3) return 2;
   return 0;
}

// ---------------------------------------------------------------------------
// Init / lifecycle
// ---------------------------------------------------------------------------

void TridentMachine::init(unsigned long currentTime) {
   currentTime_ = currentTime;
   audioHandler_.initDevices();
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.init();
#endif
   stopAllAudio();
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.setMusicDuckingGain(16);
   wavHandler_.queueSound(SOUND_EFFECT_TRIDENT_INTRO, currentTime + 5000);
#endif

   RPU_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS,
                          TriggeredSwitches);

   const auto initResult = RPU_InitializeMPU(
      RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED |
      RPU_CMD_PERFORM_MPU_TEST, SW_CREDIT_RESET);

   if ((initResult & RPU_RET_SELECTOR_SWITCH_ON) != 0) {
      queueDiagNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON, currentTime);
   } else {
      queueDiagNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_OFF, currentTime);
   }
   if ((initResult & RPU_RET_CREDIT_RESET_BUTTON_HIT) != 0) {
      queueDiagNotification(SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON, currentTime);
   }
   if ((initResult & RPU_RET_DIAGNOSTIC_REQUESTED) != 0) {
      queueDiagNotification(SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS_MODE, currentTime);
   }
   if ((initResult & RPU_RET_ORIGINAL_CODE_REQUESTED) != 0) {
      delay(100);
      queueDiagNotification(SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE, currentTime);
      while (1) ;
   }
   queueDiagNotification(SOUND_EFFECT_DIAG_STARTING_NEW_CODE, currentTime);

   disableSolenoidStack();
   setDisableFlippers(true);
}

void TridentMachine::queueDiagNotification(unsigned short notificationNum,
                                               unsigned long  currentTime) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.queuePrioritizedNotification(notificationNum, 0, 10, currentTime);
#else
   (void)notificationNum;
   (void)currentTime;
#endif
}

// ---------------------------------------------------------------------------
// PinballMachine interface
// ---------------------------------------------------------------------------

void TridentMachine::update(unsigned long currentTime) {
   currentTime_ = currentTime;
   audioHandler_.update(currentTime);
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.update(currentTime);
#endif
}

void TridentMachine::stopAllAudio() {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.stopAllAudio();
#endif
   audioHandler_.stopAllSoundFX();
}

void TridentMachine::readStoredParameters() {
   // --- Gameplay / operator settings ---
   settings_.credits = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
   if (settings_.credits > settings_.maximumCredits) settings_.credits = settings_.maximumCredits;

   settings_.freePlayMode = (readSetting(EEPROM_FREE_PLAY_BYTE, 0)) != 0;

   settings_.ballSaveNumSeconds = readSetting(EEPROM_BALL_SAVE_BYTE, 15);
   if (settings_.ballSaveNumSeconds > 20) settings_.ballSaveNumSeconds = 20;

   settings_.tournamentScoring = (readSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) != 0;

   settings_.maxTiltWarnings = readSetting(EEPROM_TILT_WARNING_BYTE, 2);
   if (settings_.maxTiltWarnings > 2) settings_.maxTiltWarnings = 2;

   uint8_t awardOverride = readSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
   if (awardOverride != 99) settings_.scoreAwardReplay = awardOverride;

   uint8_t ballsOverride = readSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
   if (ballsOverride == 3 || ballsOverride == 5) {
      settings_.ballsPerGame = ballsOverride;
   } else if (ballsOverride != 99) {
      EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
   }

   settings_.scrollingScores = (readSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) != 0;

   settings_.extraBallValue = RPU_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
   if ((settings_.extraBallValue % 1000) != 0 || settings_.extraBallValue > 100000)
      settings_.extraBallValue = 20000;

   settings_.specialValue = RPU_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
   if ((settings_.specialValue % 1000) != 0 || settings_.specialValue > 100000)
      settings_.specialValue = 40000;

   settings_.dimLevel = readSetting(EEPROM_DIM_LEVEL_BYTE, 2);
   if (settings_.dimLevel < 2 || settings_.dimLevel > 3) settings_.dimLevel = 2;
   RPU_SetDimDivisor(1, settings_.dimLevel);

   settings_.highScore  = RPU_ReadULFromEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, 10000);
   settings_.awardScores[0] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE);
   settings_.awardScores[1] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE);
   settings_.awardScores[2] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE);

   // --- Audio settings ---
   settings_.soundSelector = readSetting(EEPROM_SOUND_SELECTOR_BYTE, 3);
   switch (settings_.soundSelector) {
   case SOUND_SELECTOR_NONE:
   case SOUND_SELECTOR_ORIGINAL:
   case SOUND_SELECTOR_TRIDENT2020:
      break;
   default:
      settings_.soundSelector = SOUND_SELECTOR_TRIDENT2020;
   }
   if (settings_.soundSelector > 3) settings_.soundSelector = 3;

   settings_.musicVolume = readSetting(EEPROM_MUSIC_VOLUME_BYTE, 10);
   if (settings_.musicVolume > 10) settings_.musicVolume = 10;

   settings_.sfxVolume = readSetting(EEPROM_SFX_VOLUME_BYTE, 10);
   if (settings_.sfxVolume > 10) settings_.sfxVolume = 10;

   settings_.calloutsVolume = readSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10);
   if (settings_.calloutsVolume > 10) settings_.calloutsVolume = 10;

#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.setMusicVolume(settings_.musicVolume);
   wavHandler_.setSoundFXVolume(settings_.sfxVolume);
   wavHandler_.setNotificationsVolume(settings_.calloutsVolume);
#endif
}

void TridentMachine::setDisplay(uint8_t display, unsigned long value,
                                   bool blankLeadingZeros, uint8_t minimumDigits) {
   RPU_SetDisplay(display, value, blankLeadingZeros, minimumDigits);
}

void TridentMachine::setDisplayCredits(uint8_t credits, bool showCredits) {
   RPU_SetDisplayCredits(credits, showCredits);
}

void TridentMachine::setDisplayBallInPlay(uint8_t ball, bool showBall) {
   RPU_SetDisplayBallInPlay(ball, showBall);
}

void TridentMachine::turnOffAllLamps() {
   RPU_TurnOffAllLamps();
}

void TridentMachine::setLampState(uint8_t lamp, bool on, uint8_t dimmer, uint16_t flashRate) {
   RPU_SetLampState(lamp, on, dimmer, flashRate);
}

void TridentMachine::disableSolenoidStack() {
   RPU_DisableSolenoidStack();
}

void TridentMachine::enableSolenoidStack() {
   RPU_EnableSolenoidStack();
}

void TridentMachine::setDisableFlippers(bool disable) {
   RPU_SetDisableFlippers(disable);
}

void TridentMachine::pushToSolenoidStack(uint8_t sol, uint8_t duration, bool disableOverride) {
   RPU_PushToSolenoidStack(sol, duration, disableOverride);
}

void TridentMachine::pushToTimedSolenoidStack(uint8_t sol, uint8_t numPushes,
                                               unsigned long whenToFire, bool disableOverride) {
   RPU_PushToTimedSolenoidStack(sol, numPushes, whenToFire, disableOverride);
}

uint8_t TridentMachine::pullFirstFromSwitchStack() {
   return RPU_PullFirstFromSwitchStack();
}

bool TridentMachine::readSingleSwitchState(uint8_t sw) {
   return RPU_ReadSingleSwitchState(sw);
}

bool TridentMachine::getUpDownSwitchState() {
   return RPU_GetUpDownSwitchState();
}

void TridentMachine::setDisplayBlank(uint8_t display, uint8_t mask) {
   RPU_SetDisplayBlank(display, mask);
}

uint8_t TridentMachine::getDisplayBlank(uint8_t display) {
   return RPU_GetDisplayBlank(display);
}

void TridentMachine::cycleAllDisplays(unsigned long t, uint8_t curValue) {
   RPU_CycleAllDisplays(t, curValue);
}

uint8_t TridentMachine::magnitudeOfScore(unsigned long score) {
   if (score == 0) return 0;
   uint8_t retval = 0;
   while (score > 0) { score /= 10; retval += 1; }
   return retval;
}

uint8_t TridentMachine::getDisplayMask(uint8_t numDigits) {
   uint8_t mask = 0;
   for (uint8_t i = 0; i < numDigits; i++) mask |= (0x20 >> i);
   return mask;
}

void TridentMachine::setPlayerScore(uint8_t player, unsigned long score) {
   if (player < 4) playerScores_[player] = score;
}

void TridentMachine::showScores(uint8_t displayToUpdate, uint8_t numPlayers,
                                bool flashCurrent, bool dashCurrent,
                                unsigned long allScoresShowValue,
                                unsigned long currentTime, unsigned long lastTimeScoreChanged,
                                uint8_t overrideStatus, const unsigned long* overrideValues) {
   uint8_t       displayMask           = 0x3F;
   unsigned long displayScore          = 0;
   unsigned long overrideAnimationSeed = currentTime / 250;
   bool          scrollPhaseChanged    = false;

   uint8_t scrollPhase = (uint8_t)(((currentTime - lastTimeScoreChanged) / 250) % 16);
   if (scrollPhase != lastScrollPhase_) {
      lastScrollPhase_ = scrollPhase;
      scrollPhaseChanged = true;
   }

   bool updateLastTimeAnimated = false;

   for (uint8_t scoreCount = 0; scoreCount < 4; scoreCount++) {
      if (allScoresShowValue == 0 && overrideValues != nullptr && (overrideStatus & (0x10 << scoreCount))) {
         displayScore = overrideValues[scoreCount];
         uint8_t numDigits = magnitudeOfScore(displayScore);
         if (numDigits == 0) numDigits = 1;

         if (numDigits < (RPU_OS_NUM_DIGITS - 1) && (overrideStatus & (0x01 << scoreCount))) {
            if (overrideAnimationSeed != lastTimeOverrideAnimated_) {
               updateLastTimeAnimated = true;
               uint8_t shiftDigits = (uint8_t)(overrideAnimationSeed %
                  (((RPU_OS_NUM_DIGITS + 1) - numDigits) + ((RPU_OS_NUM_DIGITS - 1) - numDigits)));
               if (shiftDigits >= ((RPU_OS_NUM_DIGITS + 1) - numDigits)) {
                  shiftDigits = (RPU_OS_NUM_DIGITS - numDigits) * 2 - shiftDigits;
               }
               displayMask = getDisplayMask(numDigits);
               for (uint8_t digitCount = 0; digitCount < shiftDigits; digitCount++) {
                  displayScore *= 10;
                  displayMask >>= 1;
               }
               RPU_SetDisplayBlank(scoreCount, 0x00);
               RPU_SetDisplay(scoreCount, displayScore, false);
               RPU_SetDisplayBlank(scoreCount, displayMask);
            }
         } else {
            RPU_SetDisplay(scoreCount, displayScore, true);
         }
      } else {
         displayScore = (allScoresShowValue != 0) ? allScoresShowValue : playerScores_[scoreCount];

         if (displayToUpdate == 0xFF || displayToUpdate == scoreCount || displayScore > RPU_OS_MAX_DISPLAY_SCORE) {
            if (displayToUpdate == 0xFF && (scoreCount >= numPlayers && numPlayers != 0) && allScoresShowValue == 0) {
               RPU_SetDisplayBlank(scoreCount, 0x00);
               continue;
            }

            if (displayScore > RPU_OS_MAX_DISPLAY_SCORE) {
               if ((currentTime - lastTimeScoreChanged) < 4000) {
                  RPU_SetDisplay(scoreCount, displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1), false);
                  RPU_SetDisplayBlank(scoreCount, RPU_OS_ALL_DIGITS_MASK);
               } else {
                  if (scrollPhase < 11 && scrollPhaseChanged) {
                     uint8_t numDigits   = magnitudeOfScore(displayScore);
                     unsigned long tempScore = displayScore;
                     if (scrollPhase < RPU_OS_NUM_DIGITS) {
                        displayMask = RPU_OS_ALL_DIGITS_MASK;
                        for (uint8_t scrollCount = 0; scrollCount < scrollPhase; scrollCount++) {
                           displayScore = (displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1)) * 10;
                           displayMask >>= 1;
                        }
                     } else {
                        displayScore = 0;
                        displayMask  = 0x00;
                     }
                     if ((numDigits + scrollPhase) > 10) {
                        uint8_t numDigitsNeeded = (numDigits + scrollPhase) - 10;
                        for (uint8_t scrollCount = 0; scrollCount < (numDigits - numDigitsNeeded); scrollCount++) {
                           tempScore /= 10;
                        }
                        displayMask  |= getDisplayMask(magnitudeOfScore(tempScore));
                        displayScore += tempScore;
                     }
                     RPU_SetDisplayBlank(scoreCount, displayMask);
                     RPU_SetDisplay(scoreCount, displayScore);
                  }
               }
            } else {
               if (flashCurrent || dashCurrent) {
                  unsigned long seed = currentTime / 250;
                  if (seed != lastFlashOrDash_) {
                     lastFlashOrDash_ = seed;
                     if (((currentTime / 250) % 2) == 0) {
                        RPU_SetDisplayBlank(scoreCount, 0x00);
                     } else {
                        RPU_SetDisplay(scoreCount, displayScore, true, 2);
                     }
                  }
               } else {
                  RPU_SetDisplay(scoreCount, displayScore, true, 2);
               }
            }
         }
      }
   }

   if (updateLastTimeAnimated) {
      lastTimeOverrideAnimated_ = overrideAnimationSeed;
   }
}

void TridentMachine::setCoinLockout(bool lock) {
   RPU_SetCoinLockout(lock);
}

void TridentMachine::playSoundCardEffect(uint8_t sound) {
#if defined(RPU_OS_USE_SB100)
   RPU_PlaySB100(sound);
#elif defined(RPU_OS_USE_S_AND_T)
   RPU_PlaySoundSAndT(sound);
#elif defined(RPU_OS_USE_DASH51)
   RPU_PlaySoundDash51(sound);
#else
   (void)sound;
#endif
}

uint8_t TridentMachine::readByteFromEEProm(uint16_t addr) {
   return RPU_ReadByteFromEEProm(addr);
}

void TridentMachine::writeByteToEEProm(uint16_t addr, uint8_t val) {
   RPU_WriteByteToEEProm(addr, val);
}

unsigned long TridentMachine::readULFromEEProm(uint16_t addr) {
   return RPU_ReadULFromEEProm(addr);
}

void TridentMachine::writeULToEEProm(uint16_t addr, unsigned long val) {
   RPU_WriteULToEEProm(addr, val);
}

void TridentMachine::setDimDivisor(uint8_t level1, uint8_t level2) {
   RPU_SetDimDivisor(level1, level2);
}

void TridentMachine::playCallout(uint8_t track) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.playSound(track, 10);
#else
   (void)track;
#endif
}

void TridentMachine::playBackgroundSong(unsigned short songNum) {
   if (settings_.musicVolume != 0 && settings_.soundSelector == SOUND_SELECTOR_TRIDENT2020) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wavHandler_.playBackgroundSong(songNum, true);
#else
      (void)songNum;
#endif
   }
}

void TridentMachine::playBackgroundSongBasedOnBall(uint8_t ballNum) {
   if (ballNum == 1) {
      playBackgroundSong(SOUND_EFFECT_BACKGROUND_1);
   } else if (ballNum == settings_.ballsPerGame) {
      playBackgroundSong(SOUND_EFFECT_BACKGROUND_6);
   } else {
      playBackgroundSong(SOUND_EFFECT_BACKGROUND_2 + currentTime_ % 4);
   }
}

void TridentMachine::playSoundEffect(uint8_t soundEffectNum) {
   switch (settings_.soundSelector) {
   case SOUND_SELECTOR_NONE:
      return;

   case SOUND_SELECTOR_ORIGINAL:
      switch (soundEffectNum) {
      case SOUND_EFFECT_ROLLOVER:
      case SOUND_EFFECT_DT_SKILL_SHOT:
      case SOUND_EFFECT_ROLLOVER_SKILL_SHOT:
      case SOUND_EFFECT_SU_SKILL_SHOT:
      case SOUND_EFFECT_LEFT_SPINNER:
      case SOUND_EFFECT_RIGHT_SPINNER:
      case SOUND_EFFECT_DROP_TARGET:
      case SOUND_EFFECT_BALL_OVER:
         audioHandler_.queueSound(0x02, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_LEFT_INLANE:
         for (int count = 0; count < rolloverValue_; count++) {
            audioHandler_.queueSound(0x04, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_RIGHT_INLANE:
         for (int count = 0; count < 6; count++) {
            audioHandler_.queueSound((count < 3) ? 0x04 : 0x10, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_5K:
         for (int count = 0; count < 5; count++) {
            audioHandler_.queueSound(0x04, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_30K:
         for (int count = 0; count < 3; count++) {
            audioHandler_.queueSound(0x08, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_20K:
         for (int count = 0; count < 2; count++) {
            audioHandler_.queueSound(0x08, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_10K:
         for (int count = 0; count < 1; count++) {
            audioHandler_.queueSound(0x08, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_RIGHT_OUTLANE:
         for (int count = 0; count < 5; count++) {
            audioHandler_.queueSound(0x04, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_TOP_BUMPER_HIT:
      case SOUND_EFFECT_BOTTOM_BUMPER_HIT:
         audioHandler_.queueSound(0x20, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_SHOOT_AGAIN:
      case SOUND_EFFECT_PLAYER_1_UP:
      case SOUND_EFFECT_PLAYER_2_UP:
      case SOUND_EFFECT_PLAYER_3_UP:
      case SOUND_EFFECT_PLAYER_4_UP:
         audioHandler_.queueSound(0x08, currentTime_);
         audioHandler_.queueSound(0x04, currentTime_ + 75);
         audioHandler_.queueSound(0x00, currentTime_ + 175);
         break;
      case SOUND_EFFECT_BONUS_COUNT:
      case SOUND_EFFECT_2X_BONUS_COUNT:
      case SOUND_EFFECT_3X_BONUS_COUNT:
      case SOUND_EFFECT_4X_BONUS_COUNT:
      case SOUND_EFFECT_5X_BONUS_COUNT:
         audioHandler_.queueSound(0x04, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_UPPER_SLING:
      case SOUND_EFFECT_EXTRA_BALL:
      case SOUND_EFFECT_TILT_WARNING:
         audioHandler_.queueSound(0x10, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_10PT_SWITCH:
      case SOUND_EFFECT_MATCH_SPIN:
      case SOUND_EFFECT_LOWER_SLING:
         audioHandler_.queueSound(0x01, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_DROP_TARGET_CLEAR_1:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_2:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_3:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_4:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_5:
         audioHandler_.queueSound(0x08, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_FIRST_SU_SWITCH_HIT:
      case SOUND_EFFECT_SECOND_SU_SWITCH_HIT:
      case SOUND_EFFECT_THIRD_SU_SWITCH_HIT:
      case SOUND_EFFECT_FOURTH_SU_SWITCH_HIT:
      case SOUND_EFFECT_FIFTH_SU_SWITCH_HIT:
         audioHandler_.queueSound(0x04, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_ADD_CREDIT:
      case SOUND_EFFECT_GAME_OVER:
         audioHandler_.queueSound(0x08, currentTime_);
         audioHandler_.queueSound(0x04, currentTime_ + 75);
         audioHandler_.queueSound(0x02, currentTime_ + 150);
         audioHandler_.queueSound(0x01, currentTime_ + 225);
         audioHandler_.queueSound(0x08, currentTime_ + 325);
         audioHandler_.queueSound(0x04, currentTime_ + 400);
         audioHandler_.queueSound(0x02, currentTime_ + 475);
         audioHandler_.queueSound(0x01, currentTime_ + 550);
         audioHandler_.queueSound(0x00, currentTime_ + 650);
         break;
      case SOUND_EFFECT_ADD_PLAYER_1:
      case SOUND_EFFECT_ADD_PLAYER_2:
      case SOUND_EFFECT_ADD_PLAYER_3:
      case SOUND_EFFECT_ADD_PLAYER_4:
      case SOUND_EFFECT_RESCUE_FROM_THE_DEEP:
      case SOUND_EFFECT_TRIDENT_INTRO:
         audioHandler_.queueSound(0x01, currentTime_);
         audioHandler_.queueSound(0x02, currentTime_ + 75);
         audioHandler_.queueSound(0x04, currentTime_ + 150);
         audioHandler_.queueSound(0x08, currentTime_ + 225);
         audioHandler_.queueSound(0x01, currentTime_ + 325);
         audioHandler_.queueSound(0x02, currentTime_ + 400);
         audioHandler_.queueSound(0x04, currentTime_ + 475);
         audioHandler_.queueSound(0x08, currentTime_ + 550);
         audioHandler_.queueSound(0x00, currentTime_ + 650);
         break;
      }
      break;

   case SOUND_SELECTOR_TRIDENT2020:
   default:
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wavHandler_.playSound(soundEffectNum);
#endif
      break;
   }
}

// ---------------------------------------------------------------------------
// Credit and coin operations
// ---------------------------------------------------------------------------

void TridentMachine::addCredit(bool playSound, uint8_t numToAdd) {
   if (settings_.credits < settings_.maximumCredits) {
      settings_.credits += numToAdd;
      if (settings_.credits > settings_.maximumCredits) settings_.credits = settings_.maximumCredits;
      RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, settings_.credits);
      if (playSound) playSoundEffect(SOUND_EFFECT_ADD_CREDIT);
      RPU_SetDisplayCredits(settings_.credits, !settings_.freePlayMode);
      RPU_SetCoinLockout(false);
   } else {
      RPU_SetDisplayCredits(settings_.credits, !settings_.freePlayMode);
      RPU_SetCoinLockout(true);
   }
}

void TridentMachine::addSpecialCredit() {
   addCredit(false, 1);
   RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, currentTime_, true);
   RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE,
                        RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

void TridentMachine::addCoinToAudit(uint8_t chuteNum) {
   if (chuteNum > 2) return;
   unsigned short coinAuditStartByte = kChuteAuditByte[chuteNum];
   RPU_WriteULToEEProm(coinAuditStartByte, RPU_ReadULFromEEProm(coinAuditStartByte) + 1);
}

bool TridentMachine::addCoin(uint8_t chuteNum) {
   if (chuteNum > 2) return false;
   uint8_t cpcSelection = getCPCSelection(chuteNum);

   uint8_t chuteNumToUse;
   for (chuteNumToUse = 0; chuteNumToUse <= chuteNum; chuteNumToUse++) {
      if (getCPCSelection(chuteNumToUse) == cpcSelection) break;
   }

   playSoundEffect(SOUND_EFFECT_COIN_DROP_1 + (currentTime_ % 3));

   uint8_t cpcCoins          = getCPCPairCoins(cpcSelection);
   uint8_t cpcCredits        = getCPCPairCredits(cpcSelection);
   uint8_t coinProgressBefore = chuteCoinsInProgress_[chuteNumToUse];
   chuteCoinsInProgress_[chuteNumToUse] += 1;

   bool creditAdded = false;
   if (chuteCoinsInProgress_[chuteNumToUse] == cpcCoins) {
      addCredit(cpcCredits > cpcCoins ? (cpcCredits - coinProgressBefore) : cpcCredits, 1);
      chuteCoinsInProgress_[chuteNumToUse] = 0;
      creditAdded = true;
   } else if (cpcCredits > cpcCoins) {
      addCredit(true, 1);
      creditAdded = true;
   }
   return creditAdded;
}
