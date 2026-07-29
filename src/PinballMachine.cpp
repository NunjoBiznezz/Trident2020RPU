/**************************************************************************
 * PinballMachine.cpp
 *
 * Concrete machine implementation for the Trident hardware. Owns the
 * audio subsystem and MachineSettings; implements all credit / coin-mech
 * operations and RPU hardware wrappers.
 **************************************************************************/

#include "PinballMachine.h"
#include "MachineSettings.h"
#include "RPU.h"
#include "RPU_Internal.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include <Arduino.h>
#include <EEPROM.h>

// ---------------------------------------------------------------------------
// Static data
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

uint8_t PinballMachine::readSetting(int addr, uint8_t defaultValue) {
   uint8_t value = EEPROM.read(addr);
   if (value == 0xFF) {
      EEPROM.write(addr, defaultValue);
      return defaultValue;
   }
   return value;
}

uint32_t PinballMachine::readULSetting(int addr, uint32_t defaultValue) {
   uint32_t value;
   EEPROM.get(addr, value);
   if (value == 0xFFFFFFFF) {
      value = defaultValue;
      EEPROM.put(addr, value);
   }
   return value;
}

uint8_t PinballMachine::switchToChuteNum(uint8_t switchHit) {
   if (switchHit == SW_COIN_2) {
      return 1;
   }
   if (switchHit == SW_COIN_3) {
      return 2;
   }
   return 0;
}

// ---------------------------------------------------------------------------
// Init / lifecycle
// ---------------------------------------------------------------------------

void PinballMachine::init(unsigned long currentTime) {
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

   RPU_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, TriggeredSwitches);

   const auto initResult = RPU_InitializeMPU(
       RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED | RPU_CMD_PERFORM_MPU_TEST, SW_CREDIT_RESET);

   // if ((initResult & RPU_RET_SELECTOR_SWITCH_ON) != 0) {
   //    queueDiagNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON, currentTime);
   // } else {
   //    queueDiagNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_OFF, currentTime);
   // }
   // if ((initResult & RPU_RET_CREDIT_RESET_BUTTON_HIT) != 0) {
   //    queueDiagNotification(SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON, currentTime);
   // }
   // if ((initResult & RPU_RET_DIAGNOSTIC_REQUESTED) != 0) {
   //    queueDiagNotification(SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS_MODE, currentTime);
   // }
   if ((initResult & RPU_RET_ORIGINAL_CODE_REQUESTED) != 0) {
      delay(100);
      // queueDiagNotification(SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE, currentTime);
      while (1)
         ;
   }
   // queueDiagNotification(SOUND_EFFECT_DIAG_STARTING_NEW_CODE, currentTime);

   disableSolenoidStack();
   setDisableFlippers(true);
}

// void PinballMachine::queueDiagNotification(unsigned short notificationNum, unsigned long currentTime) {
// #if defined(RPU_OS_USE_WAV_TRIGGER)
//    wavHandler_.queuePrioritizedNotification(notificationNum, 0, 10, currentTime);
// #else
//    (void)notificationNum;
//    (void)currentTime;
// #endif
// }

// ---------------------------------------------------------------------------
// PinballMachine interface
// ---------------------------------------------------------------------------

void PinballMachine::readInputs() {
   RPU_DataRead(0);
}
void PinballMachine::flushOutputs() {
   RPU_Update(currentTime_);
}

void PinballMachine::update(unsigned long currentTime) {
   currentTime_ = currentTime;
   audioHandler_.update(currentTime);
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.update(currentTime);
#endif
}

void PinballMachine::stopAllAudio() {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.stopAllAudio();
#endif
   audioHandler_.stopAllSoundFX();
}

void PinballMachine::readStoredParameters() {
   // --- Gameplay / operator settings ---
   settings_.maximumCredits = readSetting(EEPROM_MAXIMUM_CREDITS_BYTE, 99);
   if (settings_.maximumCredits < 1) {
      settings_.maximumCredits = 99;
   }

   settings_.credits = readSetting(EEPROM_CREDITS_BYTE, 0);
   if (settings_.credits > settings_.maximumCredits) {
      settings_.credits = settings_.maximumCredits;
   }

   settings_.freePlayMode    = (readSetting(EEPROM_FREE_PLAY_BYTE,        0)) != 0;
   settings_.matchFeature    = (readSetting(EEPROM_MATCH_FEATURE_BYTE,    1)) != 0;
   settings_.highScoreReplay = (readSetting(EEPROM_HIGH_SCORE_REPLAY_BYTE, 1)) != 0;

   uint8_t ruleSetByte = readSetting(EEPROM_ACTIVE_RULE_SET_BYTE, (uint8_t)RuleSet::Trident2020);
   settings_.activeRuleSet = (ruleSetByte <= (uint8_t)RuleSet::Trident2020)
                             ? (RuleSet)ruleSetByte : RuleSet::Trident2020;


   settings_.tournamentScoring = (readSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) != 0;

   settings_.maxTiltWarnings = readSetting(EEPROM_TILT_WARNING_BYTE, TILT_WARNINGS_DEFAULT);
   if (settings_.maxTiltWarnings > TILT_WARNINGS_MAX) {
      settings_.maxTiltWarnings = TILT_WARNINGS_MAX;
   }


   settings_.scrollingScores = (readSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) != 0;


   settings_.dimLevel = readSetting(EEPROM_DIM_LEVEL_BYTE, 2);
   if (settings_.dimLevel < 2 || settings_.dimLevel > 3) {
      settings_.dimLevel = 2;
   }
   RPU_SetDimDivisor(1, settings_.dimLevel);

   // --- Audio settings ---
   settings_.soundSelector = readSetting(EEPROM_SOUND_SELECTOR_BYTE, 3);
   switch (settings_.soundSelector) {
   case SOUND_SELECTOR_NONE:
   case SOUND_SELECTOR_NATIVE:
   case SOUND_SELECTOR_WAV_TRIGGER:
      break;
   default:
      settings_.soundSelector = SOUND_SELECTOR_WAV_TRIGGER;
   }
   if (settings_.soundSelector > 3) {
      settings_.soundSelector = 3;
   }

   settings_.musicVolume = readSetting(EEPROM_MUSIC_VOLUME_BYTE, 10);
   if (settings_.musicVolume > 10) {
      settings_.musicVolume = 10;
   }

   settings_.sfxVolume = readSetting(EEPROM_SFX_VOLUME_BYTE, 10);
   if (settings_.sfxVolume > 10) {
      settings_.sfxVolume = 10;
   }

   settings_.calloutsVolume = readSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10);
   if (settings_.calloutsVolume > 10) {
      settings_.calloutsVolume = 10;
   }

#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.setMusicVolume(settings_.musicVolume);
   wavHandler_.setSoundFXVolume(settings_.sfxVolume);
   wavHandler_.setNotificationsVolume(settings_.calloutsVolume);
#endif

   // --- Audit counters and coin counts ---
   settings_.totalPlays   = readULSetting(EEPROM_TOTAL_PLAYS_BYTE);
   settings_.totalReplays = readULSetting(EEPROM_TOTAL_REPLAYS_BYTE);
   settings_.chute2Coins  = readULSetting(EEPROM_CHUTE_2_COINS_BYTE);
   settings_.chute1Coins  = readULSetting(EEPROM_CHUTE_1_COINS_BYTE);
   settings_.chute3Coins  = readULSetting(EEPROM_CHUTE_3_COINS_BYTE);

   this->haveSettings_ = true;
}

void PinballMachine::setDisplay(uint8_t display, unsigned long value, bool blankLeadingZeros, uint8_t minimumDigits) {
   if (display <= 3 && value > RPU_OS_MAX_DISPLAY_SCORE) {
      if (value != displayValue_[display]) {
         displayValue_[display] = value;
         displayValueSetTime_[display] = currentTime_;
         displayScrollPhase_[display] = 0xFF;
      }
      unsigned long elapsed = currentTime_ - displayValueSetTime_[display];
      if (elapsed < 4000) {
         RPU_SetDisplay(display, value % (RPU_OS_MAX_DISPLAY_SCORE + 1), false);
         RPU_SetDisplayBlank(display, RPU_OS_ALL_DIGITS_MASK);
      } else {
         uint8_t phase = (uint8_t)((elapsed / 250) % 16);
         if (phase != displayScrollPhase_[display]) {
            displayScrollPhase_[display] = phase;
            if (phase < 11) {
               unsigned long displayScore = value;
               uint8_t displayMask;
               if (phase < RPU_OS_NUM_DIGITS) {
                  displayMask = RPU_OS_ALL_DIGITS_MASK;
                  for (uint8_t sc = 0; sc < phase; sc++) {
                     displayScore = (displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1)) * 10;
                     displayMask >>= 1;
                  }
                  uint8_t numDigits = magnitudeOfScore(value);
                  if ((unsigned)(numDigits + phase) > RPU_OS_NUM_DIGITS) {
                     uint8_t numNeeded = (numDigits + phase) - RPU_OS_NUM_DIGITS;
                     unsigned long tempScore = value;
                     for (uint8_t sc = 0; sc < (numDigits - numNeeded); sc++) {
                        tempScore /= 10;
                     }
                     displayMask |= getDisplayMask(magnitudeOfScore(tempScore));
                     displayScore += tempScore;
                  }
               } else {
                  displayScore = 0;
                  displayMask = 0x00;
               }
               RPU_SetDisplayBlank(display, displayMask);
               RPU_SetDisplay(display, displayScore);
            }
         }
      }
      return;
   }
   if (display <= 3) {
      displayValue_[display] = 0;
   }
   RPU_SetDisplay(display, value, blankLeadingZeros, minimumDigits);
}

void PinballMachine::setDisplayFlash(uint8_t display, unsigned long value, uint16_t period) {
   RPU_SetDisplayFlash(display, value, currentTime_, period, 2);
}

void PinballMachine::setDisplayCredits(uint8_t credits, bool showCredits) {
   RPU_SetDisplayCredits(credits, showCredits);
}

void PinballMachine::setDisplayBallInPlay(uint8_t ball, bool showBall) {
   RPU_SetDisplayBallInPlay(ball, showBall);
}

void PinballMachine::turnOffAllLamps() {
   RPU_TurnOffAllLamps();
}

void PinballMachine::setLampState(uint8_t lamp, bool on, uint8_t dimmer, uint16_t flashRate) {
   RPU_SetLampState(lamp, on, dimmer, flashRate);
}

void PinballMachine::setLampAnimationBytes(const uint8_t* bytes, uint8_t count) {
   if (bytes) {
      RPU_SetLampAnimation(bytes, count);
   }
}

void PinballMachine::disableSolenoidStack() {
   RPU_DisableSolenoidStack();
}

void PinballMachine::enableSolenoidStack() {
   RPU_EnableSolenoidStack();
}

void PinballMachine::setDisableFlippers(bool disable) {
   RPU_SetDisableFlippers(disable);
}

void PinballMachine::pushToSolenoidStack(uint8_t sol, uint8_t duration, bool disableOverride) {
   RPU_PushToSolenoidStack(sol, duration, disableOverride);
}

void PinballMachine::pushToTimedSolenoidStack(uint8_t sol, uint8_t numPushes, unsigned long whenToFire, bool disableOverride) {
   RPU_PushToTimedSolenoidStack(sol, numPushes, whenToFire, disableOverride);
}

uint8_t PinballMachine::pullFirstFromSwitchStack() {
   return RPU_PullFirstFromSwitchStack();
}

bool PinballMachine::readSingleSwitchState(uint8_t sw) {
   return RPU_ReadSingleSwitchState(sw);
}


void PinballMachine::setDisplayBlank(uint8_t display, uint8_t mask) {
   RPU_SetDisplayBlank(display, mask);
}

uint8_t PinballMachine::getDisplayBlank(uint8_t display) {
   return RPU_GetDisplayBlank(display);
}

void PinballMachine::cycleAllDisplays(unsigned long t, uint8_t curValue) {
   RPU_CycleAllDisplays(t, curValue);
}

int           PinballMachine::getMaxLamps()             const { return RPU_MAX_LAMPS; }
uint8_t       PinballMachine::getHardwareMajorVersion() const { return RPU_OS_MAJOR_VERSION; }
uint8_t       PinballMachine::getHardwareMinorVersion() const { return RPU_OS_MINOR_VERSION; }
unsigned long PinballMachine::getMaxDisplayScore()      const { return RPU_OS_MAX_DISPLAY_SCORE; }
uint8_t       PinballMachine::getNumDisplayDigits()     const { return RPU_OS_NUM_DIGITS; }

uint8_t PinballMachine::getTotalDisplayDigits() const {
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
#  ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
   return 34;
#  else
   return 35;
#  endif
#else
   return 30;
#endif
}

uint8_t PinballMachine::magnitudeOfScore(unsigned long score) {
   if (score == 0) {
      return 0;
   }
   uint8_t retval = 0;
   while (score > 0) {
      score /= 10;
      retval += 1;
   }
   return retval;
}

uint8_t PinballMachine::getDisplayMask(uint8_t numDigits) {
   uint8_t mask = 0;
   for (uint8_t i = 0; i < numDigits; i++) {
      mask |= (0x20 >> i);
   }
   return mask;
}

void PinballMachine::setCoinLockout(bool lock) {
   RPU_SetCoinLockout(lock);
}

void PinballMachine::playNativeSound(uint8_t sound, unsigned long duration) {
   audioHandler_.queueSound(sound, currentTime_);
   audioHandler_.queueSound(SOUND_NATIVE_NONE, currentTime_ + duration);
}

void PinballMachine::decrementCredits() {
   if (settings_.credits > 0) {
      settings_.credits -= 1;
   }
   EEPROM.write(EEPROM_CREDITS_BYTE, settings_.credits);
}

void PinballMachine::saveCredits() {
   EEPROM.write(EEPROM_CREDITS_BYTE, settings_.credits);
}

void PinballMachine::recordGamePlayed() {
   settings_.totalPlays += 1;
   EEPROM.put(EEPROM_TOTAL_PLAYS_BYTE, settings_.totalPlays);
}

void PinballMachine::addReplayAudit(uint8_t count) {
   settings_.totalReplays += count;
   EEPROM.put(EEPROM_TOTAL_REPLAYS_BYTE, settings_.totalReplays);
}


void PinballMachine::acknowledgeResetScores() {
   settings_.resetScoresToClearVersion = false;
}

void PinballMachine::setDimDivisor(uint8_t level1, uint8_t level2) {
   RPU_SetDimDivisor(level1, level2);
}

void PinballMachine::playCallout(uint8_t track) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.playSound(track, 10);
#else
   (void)track;
#endif
}

void PinballMachine::playBackgroundSong(unsigned short songNum) {
   if (settings_.musicVolume != 0 && settings_.soundSelector == SOUND_SELECTOR_WAV_TRIGGER) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wavHandler_.playBackgroundSong(songNum, true);
#else
      (void)songNum;
#endif
   }
}

void PinballMachine::playSoundEffect(uint8_t soundEffectNum) {
   switch (settings_.soundSelector) {
   case SOUND_SELECTOR_NONE:
      return;

   case SOUND_SELECTOR_NATIVE:
      audioHandler_.playSoundEffect(soundEffectNum);
      break;

   case SOUND_SELECTOR_WAV_TRIGGER:
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

void PinballMachine::addCredit(bool playSound, uint8_t numToAdd) {
   if (settings_.credits < settings_.maximumCredits) {
      settings_.credits += numToAdd;
      if (settings_.credits > settings_.maximumCredits) {
         settings_.credits = settings_.maximumCredits;
      }
      EEPROM.write(EEPROM_CREDITS_BYTE, settings_.credits);
      if (playSound) {
         playSoundEffect(SOUND_EFFECT_ADD_CREDIT);
      }
      RPU_SetDisplayCredits(settings_.credits, !settings_.freePlayMode);
      RPU_SetCoinLockout(false);
   } else {
      RPU_SetDisplayCredits(settings_.credits, !settings_.freePlayMode);
      RPU_SetCoinLockout(true);
   }
}

void PinballMachine::addSpecialCredit() {
   addCredit(false, 1);
   RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, currentTime_, true);
   addReplayAudit(1);
}

void PinballMachine::addCoinToAudit(uint8_t chuteNum) {
   static const int kEeprom[3] = {
      EEPROM_CHUTE_1_COINS_BYTE, EEPROM_CHUTE_2_COINS_BYTE, EEPROM_CHUTE_3_COINS_BYTE
   };
   uint32_t* const kField[3] = {
      &settings_.chute1Coins, &settings_.chute2Coins, &settings_.chute3Coins
   };
   if (chuteNum > 2) return;
   *kField[chuteNum] += 1;
   EEPROM.put(kEeprom[chuteNum], *kField[chuteNum]);
}

bool PinballMachine::addCoin(uint8_t chuteNum) {
   if (chuteNum > 2) {
      return false;
   }
   playSoundEffect(SOUND_EFFECT_COIN_DROP_1 + (currentTime_ % 3));
   addCredit(true, 1);
   return true;
}

MachineSettings& PinballMachine::getSettings() {
   if (!haveSettings_) {
      readStoredParameters();
   }
   return settings_;
}

void PinballMachine::setLastGameResult(uint8_t numPlayers, const unsigned long* scores) {
   lastGameNumPlayers_ = numPlayers;
   for (uint8_t i = 0; i < 4; i++) {
      lastGameScores_[i] = scores[i];
   }
}
