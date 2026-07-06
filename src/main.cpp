/**************************************************************************
       This file is part of Trident2020.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 12/1/2020

    Trident2020 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Trident2020 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
*/

// Scores not ending in zero (wizard mode)
// unstructured play jackpots
// increase mode start time with new qualifier
#include "AudioHandler.h"
#include "RPU.h"
#if defined(RPU_OS_USE_WAV_TRIGGER)
#include "WavTriggerHandler.h"
#endif
#include "RPU_config.h"
#include "RPU_Internal.h"
#include "SelfTestAndAudit.h"
#include "Trident2020.h"
#include "Trident2020Game.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#include "MachineState.h"
#include "SoundEffects.h"

// Forward declarations
uint8_t ReadSetting(int setting, uint8_t defaultValue);
void PlaySoundEffect(uint8_t soundEffectNum);
void PlayBackgroundSongBasedOnBall(uint8_t ballNum);

constexpr unsigned long TRIDENT2020_MAJOR_VERSION = 2020;
constexpr unsigned long TRIDENT2020_MINOR_VERSION = 3;

// Queryable build record embedded in flash. Extract with:
//   pio run -t version -e <env>
struct __attribute__((packed)) BuildInfoRecord {
   char     magic[8];    // "TRID2020"
   uint16_t major;
   uint16_t minor;
   char     branch[32];
   char     describe[64];
   char     built[24];
};
static const BuildInfoRecord FIRMWARE_BUILD_INFO PROGMEM = {
   {'T', 'R', 'I', 'D', '2', '0', '2', '0'},
   (uint16_t)TRIDENT2020_MAJOR_VERSION,
   (uint16_t)TRIDENT2020_MINOR_VERSION,
   BUILD_GIT_BRANCH,
   BUILD_GIT_DESCRIBE,
   __DATE__ " " __TIME__,
};
   
#if defined(DEBUG_MESSAGES) && defined(DEBUG_PORT)
#  define DEBUG_MESSAGE(x) DEBUG_PORT.write(x)
#else
#  define DEBUG_MESSAGE(x)
#endif

/*********************************************************************
    Game specific code
*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
int8_t MachineState = 0;
bool MachineStateChanged = true;


/*********************************************************************

    Machine state and options

*********************************************************************/
static unsigned long HighScore = 0;
static unsigned long AwardScores[3];
static uint8_t Credits = 0;
static bool FreePlayMode = true;


static uint8_t SoundSelector = SOUND_SELECTOR_TRIDENT2020; // 0=No effects, 1=Original, 3=Trident 2020

static uint8_t MusicVolume = 10;
static uint8_t SoundEffectsVolume = 10;
static uint8_t CalloutsVolume = 10;
static uint8_t BallSaveNumSeconds = 0;
static unsigned long SoundSettingTimeout = 0;
static unsigned long ExtraBallValue = 0;
static unsigned long SpecialValue = 0;
static unsigned long CurrentTime = 0;
static uint8_t MaximumCredits = 40;
static uint8_t BallsPerGame = 3;
static uint8_t DimLevel = 2;
static uint8_t ScoreAwardReplay = 0;
static uint8_t ChuteCoinsInProgress[3] = {0, 0, 0};
static bool HighScoreReplay = true;
static bool MatchFeature = true;
// static uint8_t SpecialLightAward = 0;
static bool TournamentScoring = false;
static bool ResetScoresToClearVersion = false;
static bool ScrollingScores = true;
static uint8_t MaxTiltWarnings = 2;
static uint8_t SharpShooterStartBonus = 3;
static uint8_t TargetSpecialBonus = 4;
static uint8_t StandupSpecialLevel = 2;

/*********************************************************************
 * Audio
 *********************************************************************/
static AudioHandler audioHandler;
#if defined(RPU_OS_USE_WAV_TRIGGER)
static WavTriggerHandler wavHandler;
#endif

static void StopAllAudio() {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler.stopAllAudio();
#endif
   audioHandler.stopAllSoundFX();
}

static Trident2020Game game;

static GameContext g_ctx = {
   &Credits,
   &MaximumCredits,
   &BallsPerGame,
   &BallSaveNumSeconds,
   &FreePlayMode,
   &TournamentScoring,
   &ScrollingScores,
   &HighScore,
   AwardScores,
   &ScoreAwardReplay,
   &MaxTiltWarnings,
   &ResetScoresToClearVersion,
   &ExtraBallValue,
   &SpecialValue,
   &HighScoreReplay,
   &MatchFeature,
   &SharpShooterStartBonus,
   &TargetSpecialBonus,
   &StandupSpecialLevel,
};

void ReadStoredParameters() {
   HighScore = RPU_ReadULFromEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, 10000);
   Credits = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
   if (Credits > MaximumCredits) {
      Credits = MaximumCredits;
   }

   ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
   FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) != 0;

   BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
   if (BallSaveNumSeconds > 20) {
      BallSaveNumSeconds = 20;
   }

   SoundSelector = ReadSetting(EEPROM_SOUND_SELECTOR_BYTE, 3);
   switch (SoundSelector) {
   case SOUND_SELECTOR_NONE:
   case SOUND_SELECTOR_ORIGINAL:
   case SOUND_SELECTOR_TRIDENT2020:
      // All valid values, thanks
      break;
   default:
      SoundSelector = SOUND_SELECTOR_TRIDENT2020;
   }

   if (SoundSelector > 3) {
      SoundSelector = 3;
   }

   MusicVolume = ReadSetting(EEPROM_MUSIC_VOLUME_BYTE, 10);
   if (MusicVolume > 10) {
      MusicVolume = 10;
   }

   SoundEffectsVolume = ReadSetting(EEPROM_SFX_VOLUME_BYTE, 10);
   if (SoundEffectsVolume > 10) {
      SoundEffectsVolume = 10;
   }

   CalloutsVolume = ReadSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10);
   if (CalloutsVolume > 10) {
      CalloutsVolume = 10;
   }

#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler.setMusicVolume(MusicVolume);
   wavHandler.setSoundFXVolume(SoundEffectsVolume);
   wavHandler.setNotificationsVolume(CalloutsVolume);
#endif

   TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) != 0;

   MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
   if (MaxTiltWarnings > 2) {
      MaxTiltWarnings = 2;
   }

   uint8_t awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
   if (awardOverride != 99) {
      ScoreAwardReplay = awardOverride;
   }

   uint8_t ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
   if (ballsOverride == 3 || ballsOverride == 5) {
      BallsPerGame = ballsOverride;
   } else {
      if (ballsOverride != 99) {
         EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
      }
   }

   ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) != 0;

   ExtraBallValue = RPU_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
   if ((ExtraBallValue % 1000) != 0 || ExtraBallValue > 100000) {
      ExtraBallValue = 20000;
   }

   SpecialValue = RPU_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
   if ((SpecialValue % 1000) != 0 || SpecialValue > 100000) {
      SpecialValue = 40000;
   }

   DimLevel = ReadSetting(EEPROM_DIM_LEVEL_BYTE, 2);
   if (DimLevel < 2 || DimLevel > 3) {
      DimLevel = 2;
   }
   RPU_SetDimDivisor(1, DimLevel);

   AwardScores[0] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE);
   AwardScores[1] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE);
   AwardScores[2] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE);
}

void QueueDIAGNotification(unsigned short notificationNum) {
   // This is optional, but the machine can play an audio message at boot
   // time to indicate any errors and whether it's going to boot to original
   // or new code.
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler.queuePrioritizedNotification(notificationNum, 0, 10, CurrentTime);
#else
   (void)notificationNum;
#endif
}

void setup() {
   // Opaque to LTO — prevents --gc-sections from discarding FIRMWARE_BUILD_INFO.
   __asm__ volatile("" :: "r"(&FIRMWARE_BUILD_INFO) : "memory");

   #if defined(DEBUG_PORT)
      DEBUG_PORT.begin(115200);
   #  if defined(DEBUG_MESSAGES)
      DEBUG_PORT.print(F("Trident2020 v"));
      DEBUG_PORT.print(TRIDENT2020_MAJOR_VERSION);
      DEBUG_PORT.print('.');
      DEBUG_PORT.println(TRIDENT2020_MINOR_VERSION);
      DEBUG_PORT.print(F("Branch:   "));
      DEBUG_PORT.println(F(BUILD_GIT_BRANCH));
      DEBUG_PORT.print(F("Describe: "));
      DEBUG_PORT.println(F(BUILD_GIT_DESCRIBE));
      DEBUG_PORT.print(F("Built:    "));
      DEBUG_PORT.println(F(__DATE__ " " __TIME__));
   #  endif
   #endif


   CurrentTime = millis();
   audioHandler.initDevices();
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler.init();
#endif
   StopAllAudio();

   // Tell the OS about game-specific lights and switches
   RPU_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, TriggeredSwitches);

   // Set up the chips and interrupts
   const auto initResult = RPU_InitializeMPU(
       RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED | RPU_CMD_PERFORM_MPU_TEST, SW_CREDIT_RESET);

   if ((initResult & RPU_RET_SELECTOR_SWITCH_ON) != 0) {
      QueueDIAGNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON);
   } else {
      QueueDIAGNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_OFF);
   }
   if ((initResult & RPU_RET_CREDIT_RESET_BUTTON_HIT) != 0) {
      QueueDIAGNotification(SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON);
   }

   if ((initResult & RPU_RET_DIAGNOSTIC_REQUESTED) != 0) {
      QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS_MODE);
      // Run diagnostics here:
   }

   if ((initResult & RPU_RET_ORIGINAL_CODE_REQUESTED) != 0) {
      delay(100);
      QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE);
//#if defined(RPU_OS_USE_WAV_TRIGGER)
//      while (wavHandler.update(millis()))
//         ;
//#endif
      // Arduino should hang if original code is running
      while (1)
         ;
   }
   QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_NEW_CODE);

   RPU_DisableSolenoidStack();
   RPU_SetDisableFlippers(true);

   // Read parameters from EEProm
   ReadStoredParameters();

   game.setScore(0, TRIDENT2020_MAJOR_VERSION);
   game.setCurrentPlayerScore(TRIDENT2020_MAJOR_VERSION);
   game.setScore(1, TRIDENT2020_MINOR_VERSION);
   game.setScore(2, RPU_OS_MAJOR_VERSION);
   game.setScore(3, RPU_OS_MINOR_VERSION);
   ResetScoresToClearVersion = true;

   CurrentTime = millis();
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler.setMusicDuckingGain(16);
   wavHandler.queueSound(SOUND_EFFECT_TRIDENT_INTRO, CurrentTime + 5000);
#endif
}

uint8_t ReadSetting(int setting, uint8_t defaultValue) {
   uint8_t value = EEPROM.read(setting);
   if (value == 0xFF) {
      EEPROM.write(setting, defaultValue);
      return defaultValue;
   }
   return value;
}

static const unsigned short ChuteAuditByte[] = {RPU_CHUTE_1_COINS_START_BYTE, RPU_CHUTE_2_COINS_START_BYTE, RPU_CHUTE_3_COINS_START_BYTE};

void AddCoinToAudit(uint8_t chuteNum) {
   if (chuteNum > 2) {
      return;
   }
   unsigned short coinAuditStartByte = ChuteAuditByte[chuteNum];
   RPU_WriteULToEEProm(coinAuditStartByte, RPU_ReadULFromEEProm(coinAuditStartByte) + 1);
}

void AddCredit(boolean playSound = false, uint8_t numToAdd = 1) {
   if (Credits < MaximumCredits) {
      Credits += numToAdd;
      if (Credits > MaximumCredits) {
         Credits = MaximumCredits;
      }
      RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
      if (playSound) {
         PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
      }
      RPU_SetDisplayCredits(Credits, !FreePlayMode);
      RPU_SetCoinLockout(false);
   } else {
      RPU_SetDisplayCredits(Credits, !FreePlayMode);
      RPU_SetCoinLockout(true);
   }
}

uint8_t SwitchToChuteNum(uint8_t switchHit) {
   uint8_t chuteNum = 0;
   if (switchHit == SW_COIN_2) {
      chuteNum = 1;
   } else if (switchHit == SW_COIN_3) {
      chuteNum = 2;
   }
   return chuteNum;
}

bool AddCoin(uint8_t chuteNum) {
   bool creditAdded = false;
   if (chuteNum > 2) {
      return false;
   }
   uint8_t cpcSelection = GetCPCSelection(chuteNum);

   // Find the lowest chute num with the same ratio selection
   // and use that ChuteCoinsInProgress counter
   uint8_t chuteNumToUse;
   for (chuteNumToUse = 0; chuteNumToUse <= chuteNum; chuteNumToUse++) {
      if (GetCPCSelection(chuteNumToUse) == cpcSelection) {
         break;
      }
   }

   PlaySoundEffect(SOUND_EFFECT_COIN_DROP_1 + (CurrentTime % 3));

   uint8_t cpcCoins = GetCPCCoins(cpcSelection);
   uint8_t cpcCredits = GetCPCCredits(cpcSelection);
   uint8_t coinProgressBefore = ChuteCoinsInProgress[chuteNumToUse];
   ChuteCoinsInProgress[chuteNumToUse] += 1;

   if (ChuteCoinsInProgress[chuteNumToUse] == cpcCoins) {
      if (cpcCredits > cpcCoins) {
         AddCredit(cpcCredits - (coinProgressBefore));
      } else {
         AddCredit(cpcCredits);
      }
      ChuteCoinsInProgress[chuteNumToUse] = 0;
      creditAdded = true;
   } else {
      if (cpcCredits > cpcCoins) {
         AddCredit(1);
         creditAdded = true;
      } else {
      }
   }

   return creditAdded;
}

void AddSpecialCredit() {
   AddCredit(false, 1);
   RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
   RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

enum AdjustmentType_t {
   ADJ_TYPE_LIST = 1,
   ADJ_TYPE_MIN_MAX = 2,
   ADJ_TYPE_MIN_MAX_DEFAULT = 3,
   ADJ_TYPE_SCORE = 4,
   ADJ_TYPE_SCORE_WITH_DEFAULT = 5,
   ADJ_TYPE_SCORE_NO_DEFAULT = 6
};

uint8_t AdjustmentType = 0;
uint8_t NumAdjustmentValues = 0;
uint8_t AdjustmentValues[8];
unsigned long AdjustmentScore;
uint8_t* CurrentAdjustmentByte = NULL;
unsigned long* CurrentAdjustmentUL = NULL;
uint8_t CurrentAdjustmentStorageByte = 0;
uint8_t TempValue = 0;

const uint8_t SelfTestStateToCalloutMap[] = {136, 137, 135, 134, 133, 140, 141, 142, 139, 143, 144, 145, 146, 147, 148, 149, 138, 150,
                                             151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 171, 0};

const uint8_t SoundSelectorToCalloutsMap[] = {190, 191, 199, 197, 198, 196};

int RunSelfTest(int curState, bool curStateChanged) {
   int returnState = curState;
   game.setNumPlayers(0);

   if (curStateChanged) {
      // Send a stop-all command and reset the sample-rate offset, in case we have
      //  reset while the WAV Trigger was already playing.
      StopAllAudio();
      unsigned short modeMapping = SelfTestStateToCalloutMap[-1 - curState];
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wavHandler.playSound(modeMapping, 10);
#else
      (void)modeMapping;
#endif
      SoundSettingTimeout = 0;
   } else {
      if (SoundSettingTimeout != 0 && CurrentTime > SoundSettingTimeout) {
         SoundSettingTimeout = 0;
         StopAllAudio();
      }
   }

   // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
   // Any that's less, is machine specific, so we handle it here.
   if (curState >= MACHINE_STATE_TEST_DONE) {
      uint8_t cpcSelection = 0xFF;
      uint8_t chuteNum = 0xFF;
      if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_1) {
         chuteNum = 0;
      }
      if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_2) {
         chuteNum = 1;
      }
      if (curState == MACHINE_STATE_ADJUST_CPC_CHUTE_3) {
         chuteNum = 2;
      }
      if (chuteNum != 0xFF) {
         cpcSelection = GetCPCSelection(chuteNum);
      }
      returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);
      if (chuteNum != 0xFF) {
         if (cpcSelection != GetCPCSelection(chuteNum)) {
            uint8_t newCPC = GetCPCSelection(chuteNum);
            StopAllAudio();
#if defined(RPU_OS_USE_WAV_TRIGGER)
            wavHandler.playSound(SOUND_EFFECT_SELF_TEST_CPC_START + newCPC, 10);
#endif
         }
      }
   } else {
      uint8_t curSwitch = RPU_PullFirstFromSwitchStack();

      if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
         SetLastSelfTestChangedTime(CurrentTime);
         returnState -= 1;
      }

      if (curSwitch == SW_SLAM) {
         returnState = MACHINE_STATE_ATTRACT;
      }

      if (curStateChanged) {
         for (int count = 0; count < 4; count++) {
            RPU_SetDisplay(count, 0);
            RPU_SetDisplayBlank(count, 0x00);
         }
         RPU_SetDisplayCredits(MACHINE_STATE_TEST_SOUNDS - curState);
         RPU_SetDisplayBallInPlay(0, false);
         CurrentAdjustmentByte = NULL;
         CurrentAdjustmentUL = NULL;
         CurrentAdjustmentStorageByte = 0;

         AdjustmentType = ADJ_TYPE_MIN_MAX;
         AdjustmentValues[0] = 0;
         AdjustmentValues[1] = 1;
         TempValue = 0;

         switch (curState) {
         case MACHINE_STATE_ADJUST_FREEPLAY:
            CurrentAdjustmentByte = (uint8_t*)&FreePlayMode;
            CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
            break;
         case MACHINE_STATE_ADJUST_BALL_SAVE:
            AdjustmentType = ADJ_TYPE_LIST;
            NumAdjustmentValues = 5;
            AdjustmentValues[1] = 5;
            AdjustmentValues[2] = 10;
            AdjustmentValues[3] = 15;
            AdjustmentValues[4] = 20;
            CurrentAdjustmentByte = &BallSaveNumSeconds;
            CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK:
            AdjustmentType = ADJ_TYPE_MIN_MAX;
            AdjustmentValues[1] = 5;
            CurrentAdjustmentByte = &SoundSelector;
            CurrentAdjustmentStorageByte = EEPROM_SOUND_SELECTOR_BYTE;
            break;

         case MACHINE_STATE_ADJUST_MUSIC_VOLUME:
            AdjustmentType = ADJ_TYPE_MIN_MAX;
            AdjustmentValues[1] = 10;
            CurrentAdjustmentByte = &MusicVolume;
            CurrentAdjustmentStorageByte = EEPROM_MUSIC_VOLUME_BYTE;
            break;

         case MACHINE_STATE_ADJUST_SFX_VOLUME:
            AdjustmentType = ADJ_TYPE_MIN_MAX;
            AdjustmentValues[1] = 10;
            CurrentAdjustmentByte = &SoundEffectsVolume;
            CurrentAdjustmentStorageByte = EEPROM_SFX_VOLUME_BYTE;
            break;

         case MACHINE_STATE_ADJUST_CALLOUTS_VOLUME:
            AdjustmentType = ADJ_TYPE_MIN_MAX;
            AdjustmentValues[1] = 10;
            CurrentAdjustmentByte = &CalloutsVolume;
            CurrentAdjustmentStorageByte = EEPROM_CALLOUTS_VOLUME_BYTE;
            break;

         case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
            CurrentAdjustmentByte = (uint8_t*)&TournamentScoring;
            CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
            break;

         case MACHINE_STATE_ADJUST_TILT_WARNING:
            AdjustmentValues[1] = 2;
            CurrentAdjustmentByte = &MaxTiltWarnings;
            CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
            break;

         case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
            AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
            AdjustmentValues[1] = 7;
            CurrentAdjustmentByte = &ScoreAwardReplay;
            CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
            AdjustmentType = ADJ_TYPE_LIST;
            NumAdjustmentValues = 3;
            AdjustmentValues[0] = 3;
            AdjustmentValues[1] = 5;
            AdjustmentValues[2] = 99;
            CurrentAdjustmentByte = &BallsPerGame;
            CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
            CurrentAdjustmentByte = (uint8_t*)&ScrollingScores;
            CurrentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
            break;

         case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
            AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
            CurrentAdjustmentUL = &ExtraBallValue;
            CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
            AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
            CurrentAdjustmentUL = &SpecialValue;
            CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_BYTE;
            break;

         case MACHINE_STATE_ADJUST_DIM_LEVEL:
            AdjustmentType = ADJ_TYPE_LIST;
            NumAdjustmentValues = 2;
            AdjustmentValues[0] = 2;
            AdjustmentValues[1] = 3;
            CurrentAdjustmentByte = &DimLevel;
            CurrentAdjustmentStorageByte = EEPROM_DIM_LEVEL_BYTE;
            for (int count = 0; count < 10; count++) {
               RPU_SetLampState(BONUS_1 + count, true, 1);
            }
            break;

         case MACHINE_STATE_ADJUST_DONE:
            returnState = MACHINE_STATE_ATTRACT;
            break;
         }
      }

      // Change value, if the switch is hit
      if (curSwitch == SW_CREDIT_RESET) {
         if (CurrentAdjustmentByte != nullptr && (AdjustmentType == ADJ_TYPE_MIN_MAX || AdjustmentType == ADJ_TYPE_MIN_MAX_DEFAULT)) {
            uint8_t curVal = *CurrentAdjustmentByte;
            curVal += 1;
            if (curVal > AdjustmentValues[1]) {
               if (AdjustmentType == ADJ_TYPE_MIN_MAX) {
                  curVal = AdjustmentValues[0];
               } else {
                  if (curVal > 99) {
                     curVal = AdjustmentValues[0];
                  } else {
                     curVal = 99;
                  }
               }
            }
            *CurrentAdjustmentByte = curVal;
            if (CurrentAdjustmentStorageByte != 0) {
               EEPROM.write(CurrentAdjustmentStorageByte, curVal);
            }
         } else if (CurrentAdjustmentByte != nullptr && AdjustmentType == ADJ_TYPE_LIST) {
            uint8_t valCount = 0;
            uint8_t curVal = *CurrentAdjustmentByte;
            uint8_t newIndex = 0;
            for (valCount = 0; valCount < (NumAdjustmentValues - 1); valCount++) {
               if (curVal == AdjustmentValues[valCount]) {
                  newIndex = valCount + 1;
               }
            }
            *CurrentAdjustmentByte = AdjustmentValues[newIndex];
            if (CurrentAdjustmentStorageByte != 0) {
               EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
            }
         } else if (CurrentAdjustmentUL != nullptr && (AdjustmentType == ADJ_TYPE_SCORE_WITH_DEFAULT || AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT)) {
            unsigned long curVal = *CurrentAdjustmentUL;
            curVal += 5000;
            if (curVal > 100000) {
               curVal = 0;
            }
            if (AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT && curVal == 0) {
               curVal = 5000;
            }
            *CurrentAdjustmentUL = curVal;
            if (CurrentAdjustmentStorageByte != 0) {
               RPU_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
            }
         }

         if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
            RPU_SetDimDivisor(1, DimLevel);
         }
      }

      // Show current value
      if (CurrentAdjustmentByte != NULL) {
         RPU_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
      } else if (CurrentAdjustmentUL != NULL) {
         RPU_SetDisplay(0, (*CurrentAdjustmentUL), true);
      }
   }

   if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
      for (int count = 0; count < 10; count++) {
         RPU_SetLampState(BONUS_1 + count, true, (CurrentTime / 1000) % 2);
      }
   }

   if (returnState == MACHINE_STATE_ATTRACT) {
      // If any variables have been set to non-override (99), return
      // them to dip switch settings
      // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
      //    DecodeDIPSwitchParameters();
      ReadStoredParameters();
   }

   return returnState;
}

////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////

#if defined(RPU_USE_WAV_TRIGGER) || defined(RPU_USE_WAV_TRIGGER_1p3)
uint8_t CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif

void PlayBackgroundSong(unsigned short songNum) {
   if ((MusicVolume != 0) && (SoundSelector == SOUND_SELECTOR_TRIDENT2020)) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wavHandler.playBackgroundSong(songNum, true);
#endif
   }
}

void PlayBackgroundSongBasedOnBall(uint8_t ballNum) {
   if (ballNum == 1) {
      PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1);
   } else if (ballNum == BallsPerGame) {
      PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_6);
   } else {
      PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_2 + CurrentTime % 4);
   }
}

unsigned long NextSoundEffectTime = 0;

void PlaySoundEffect(uint8_t soundEffectNum) {
   switch (SoundSelector) {
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
         audioHandler.queueSound(0x02, CurrentTime);
         audioHandler.queueSound(0x00, CurrentTime + 75);
         break;
      case SOUND_EFFECT_LEFT_INLANE:
         for (int count = 0; count < game.getRolloverValue(); count++) {
            audioHandler.queueSound(0x04, CurrentTime + 200 * count);
            audioHandler.queueSound(0x00, CurrentTime + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_RIGHT_INLANE:
         for (int count = 0; count < 6; count++) {
            audioHandler.queueSound((count < 3) ? 0x04 : 0x10, CurrentTime + 200 * count);
            audioHandler.queueSound(0x00, CurrentTime + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_5K:
         for (int count = 0; count < 5; count++) {
            audioHandler.queueSound(0x04, CurrentTime + 200 * count);
            audioHandler.queueSound(0x00, CurrentTime + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_30K:
         for (int count = 0; count < 3; count++) {
            audioHandler.queueSound(0x08, CurrentTime + 200 * count);
            audioHandler.queueSound(0x00, CurrentTime + 75 + (200 * count));
         }
         break;

      case SOUND_EFFECT_SAUCER_HIT_20K:
         for (int count = 0; count < 2; count++) {
            audioHandler.queueSound(0x08, CurrentTime + 200 * count);
            audioHandler.queueSound(0x00, CurrentTime + 75 + (200 * count));
         }
         break;

      case SOUND_EFFECT_SAUCER_HIT_10K:
         for (int count = 0; count < 1; count++) {
            audioHandler.queueSound(0x08, CurrentTime + 200 * count);
            audioHandler.queueSound(0x00, CurrentTime + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_RIGHT_OUTLANE:
         for (int count = 0; count < 5; count++) {
            audioHandler.queueSound(0x04, CurrentTime + 200 * count);
            audioHandler.queueSound(0x00, CurrentTime + 75 + (200 * count));
         }
         break;

      case SOUND_EFFECT_TOP_BUMPER_HIT:
      case SOUND_EFFECT_BOTTOM_BUMPER_HIT:
         audioHandler.queueSound(0x20, CurrentTime);
         audioHandler.queueSound(0x00, CurrentTime + 75);
         break;

      case SOUND_EFFECT_SHOOT_AGAIN:
      case SOUND_EFFECT_PLAYER_1_UP:
      case SOUND_EFFECT_PLAYER_2_UP:
      case SOUND_EFFECT_PLAYER_3_UP:
      case SOUND_EFFECT_PLAYER_4_UP:
         audioHandler.queueSound(0x08, CurrentTime);
         audioHandler.queueSound(0x04, CurrentTime + 75);
         audioHandler.queueSound(0x00, CurrentTime + 175);
         break;

      case SOUND_EFFECT_BONUS_COUNT:
      case SOUND_EFFECT_2X_BONUS_COUNT:
      case SOUND_EFFECT_3X_BONUS_COUNT:
      case SOUND_EFFECT_4X_BONUS_COUNT:
      case SOUND_EFFECT_5X_BONUS_COUNT:
         audioHandler.queueSound(0x04, CurrentTime);
         audioHandler.queueSound(0x00, CurrentTime + 75);
         break;

      case SOUND_EFFECT_UPPER_SLING:
      case SOUND_EFFECT_EXTRA_BALL:
      case SOUND_EFFECT_TILT_WARNING:
         audioHandler.queueSound(0x10, CurrentTime);
         audioHandler.queueSound(0x00, CurrentTime + 75);
         break;
      case SOUND_EFFECT_10PT_SWITCH:
      case SOUND_EFFECT_MATCH_SPIN:
      case SOUND_EFFECT_LOWER_SLING:
         audioHandler.queueSound(0x01, CurrentTime);
         audioHandler.queueSound(0x00, CurrentTime + 75);
         break;

      case SOUND_EFFECT_DROP_TARGET_CLEAR_1:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_2:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_3:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_4:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_5:
         audioHandler.queueSound(0x08, CurrentTime);
         audioHandler.queueSound(0x00, CurrentTime + 75);
         break;

      case SOUND_EFFECT_FIRST_SU_SWITCH_HIT:
      case SOUND_EFFECT_SECOND_SU_SWITCH_HIT:
      case SOUND_EFFECT_THIRD_SU_SWITCH_HIT:
      case SOUND_EFFECT_FOURTH_SU_SWITCH_HIT:
      case SOUND_EFFECT_FIFTH_SU_SWITCH_HIT:
         audioHandler.queueSound(0x04, CurrentTime);
         audioHandler.queueSound(0x00, CurrentTime + 75);
         break;

      case SOUND_EFFECT_ADD_CREDIT:
      case SOUND_EFFECT_GAME_OVER:
         audioHandler.queueSound(0x08, CurrentTime);
         audioHandler.queueSound(0x04, CurrentTime + 75);
         audioHandler.queueSound(0x02, CurrentTime + 150);
         audioHandler.queueSound(0x01, CurrentTime + 225);
         audioHandler.queueSound(0x08, CurrentTime + 325);
         audioHandler.queueSound(0x04, CurrentTime + 400);
         audioHandler.queueSound(0x02, CurrentTime + 475);
         audioHandler.queueSound(0x01, CurrentTime + 550);
         audioHandler.queueSound(0x00, CurrentTime + 650);
         break;

      case SOUND_EFFECT_ADD_PLAYER_1:
      case SOUND_EFFECT_ADD_PLAYER_2:
      case SOUND_EFFECT_ADD_PLAYER_3:
      case SOUND_EFFECT_ADD_PLAYER_4:
      case SOUND_EFFECT_RESCUE_FROM_THE_DEEP:
      case SOUND_EFFECT_TRIDENT_INTRO:
         audioHandler.queueSound(0x01, CurrentTime);
         audioHandler.queueSound(0x02, CurrentTime + 75);
         audioHandler.queueSound(0x04, CurrentTime + 150);
         audioHandler.queueSound(0x08, CurrentTime + 225);
         audioHandler.queueSound(0x01, CurrentTime + 325);
         audioHandler.queueSound(0x02, CurrentTime + 400);
         audioHandler.queueSound(0x04, CurrentTime + 475);
         audioHandler.queueSound(0x08, CurrentTime + 550);
         audioHandler.queueSound(0x00, CurrentTime + 650);
         break;
      }
      break;

   case SOUND_SELECTOR_TRIDENT2020:
   default:
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wavHandler.playSound(soundEffectNum);
#endif
      break;
   }
}

////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////

unsigned long AttractLastLadderTime = 0;
uint8_t AttractLastLadderBonus = 0;
unsigned long AttractLastStarTime = 0;
uint8_t AttractLastHeadMode = 255;
uint8_t AttractLastPlayfieldMode = 255;
bool InAttractMode = false;

int RunAttractMode(int curState, bool curStateChanged) {
   int returnState = curState;

   if (curStateChanged) {
#ifdef RPU_OS_USE_SB100
      RPU_PlaySB100(0);
#endif
      RPU_DisableSolenoidStack();
      RPU_TurnOffAllLamps();
      RPU_SetDisableFlippers(true);
      DEBUG_MESSAGE("Entering Attract Mode\n\r");

      AttractLastHeadMode = 0;
      AttractLastPlayfieldMode = 0;
   }

   // Alternate displays between high score and blank
   if (CurrentTime < 16000) {
      if (AttractLastHeadMode != 1) {
         game.showPlayerScores(0xFF, false, false);
         game.setPlayerLamps(0);
         RPU_SetDisplayCredits(Credits, true);
         RPU_SetDisplayBallInPlay(0, true);
      }
   } else if ((CurrentTime / 8000) % 2 == 0) {
      if (AttractLastHeadMode != 2) {
         RPU_SetLampState(HIGH_SCORE_TO_DATE, true, 0, 250);
         RPU_SetLampState(GAME_OVER, false);
         game.setPlayerLamps(0);
         game.markScoreChanged(CurrentTime);
      }
      AttractLastHeadMode = 2;
      game.showPlayerScores(0xFF, false, false, HighScore);
   } else {
      if (AttractLastHeadMode != 3) {
         if (CurrentTime < 32000) {
            for (int count = 0; count < 4; count++) {
               game.setScore(count, 0);
            }
            game.setNumPlayers(0);
         }
         RPU_SetLampState(HIGH_SCORE_TO_DATE, false);
         RPU_SetLampState(GAME_OVER, true);
         RPU_SetDisplayCredits(Credits, true);
         RPU_SetDisplayBallInPlay(0, true);
         game.markScoreChanged(CurrentTime);
      }
      game.showPlayerScores(0xFF, false, false);

      game.setPlayerLamps(((CurrentTime / 250) % 4) + 1);
      AttractLastHeadMode = 3;
   }

   if ((CurrentTime / 10000) % 3 < 2) {
      if (AttractLastPlayfieldMode != 1) {
         RPU_TurnOffAllLamps();
         game.setGameMode(GAME_MODE_SKILL_SHOT);
      }
      game.showSaucerLamps();
      game.showDropTargetLamps();
      game.showStandupTargetLamps();
      game.showLeftSpinnerLamps();
      game.showLeftLaneLamps();

      AttractLastPlayfieldMode = 1;
   } else {
      if (AttractLastPlayfieldMode != 2) {
         RPU_TurnOffAllLamps();
         AttractLastLadderBonus = 1;
         AttractLastLadderTime = CurrentTime;
      }
      if ((CurrentTime - AttractLastLadderTime) > 200) {
         AttractLastLadderBonus += 1;
         AttractLastLadderTime = CurrentTime;
         game.showBonusOnTree(AttractLastLadderBonus % MAX_DISPLAY_BONUS);
      }

      AttractLastPlayfieldMode = 2;
   }

   uint8_t switchHit;
   while ((switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY) {
      if (switchHit == SW_CREDIT_RESET) {
         if (game.addPlayer(true, g_ctx)) {
            returnState = MACHINE_STATE_INIT_GAMEPLAY;
         }
      }
      if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
         AddCoinToAudit(switchHit);
         AddCredit(true, 1);
      }
      if (switchHit == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
         returnState = MACHINE_STATE_TEST_LAMPS;
         SetLastSelfTestChangedTime(CurrentTime);
      }
   }

   return returnState;
}

void loop() {
   RPU_DataRead(0);
   CurrentTime = millis();
   int newMachineState = MachineState;

   if (MachineState < 0) {
      newMachineState = RunSelfTest(MachineState, MachineStateChanged);
   } else if (MachineState == MACHINE_STATE_ATTRACT) {
      newMachineState = RunAttractMode(MachineState, MachineStateChanged);
   } else {
      newMachineState = game.run(MachineState, MachineStateChanged, CurrentTime, g_ctx);
   }

   if (newMachineState != MachineState) {
      MachineState = newMachineState;
      MachineStateChanged = true;
   } else {
      MachineStateChanged = false;
   }

   audioHandler.update(CurrentTime);
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler.update(CurrentTime);
#endif
   RPU_Update(CurrentTime);
}
