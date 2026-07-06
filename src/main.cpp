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
#include "AttractMode.h"
#include "MachineMode.h"
#include "MachineSettings.h"
#include "MachineState.h"
#include "RPU.h"
#include "RPU_config.h"
#include "RPU_Internal.h"
#include "SelfTestAndAudit.h"
#include "SelfTestMode.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include "Trident2020Game.h"
#include "TridentMachine.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

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
    Top-level objects — settings live inside machineOps
*********************************************************************/
static unsigned long CurrentTime = 0;

static TridentMachine  machineOps;
static Trident2020Game game;
static SelfTestMode    selfTestMode;
static AttractMode     attractMode;

static TopState     topState   = TopState::Attract;
static MachineMode* activeMode = &attractMode;


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
   machineOps.init(CurrentTime);

   RPU_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS,
                          TriggeredSwitches);

   const auto initResult = RPU_InitializeMPU(
      RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED |
      RPU_CMD_PERFORM_MPU_TEST, SW_CREDIT_RESET);

   if ((initResult & RPU_RET_SELECTOR_SWITCH_ON) != 0) {
      machineOps.queueDiagNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON, CurrentTime);
   } else {
      machineOps.queueDiagNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_OFF, CurrentTime);
   }
   if ((initResult & RPU_RET_CREDIT_RESET_BUTTON_HIT) != 0) {
      machineOps.queueDiagNotification(SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON, CurrentTime);
   }
   if ((initResult & RPU_RET_DIAGNOSTIC_REQUESTED) != 0) {
      machineOps.queueDiagNotification(SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS_MODE, CurrentTime);
   }
   if ((initResult & RPU_RET_ORIGINAL_CODE_REQUESTED) != 0) {
      delay(100);
      machineOps.queueDiagNotification(SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE, CurrentTime);
      while (1) ;
   }
   machineOps.queueDiagNotification(SOUND_EFFECT_DIAG_STARTING_NEW_CODE, CurrentTime);

   RPU_DisableSolenoidStack();
   RPU_SetDisableFlippers(true);

   machineOps.readStoredParameters();

   MachineSettings& s = machineOps.settings();
   game.setSettings(s);
   game.setMachine(machineOps);
   selfTestMode.setDependencies(game, machineOps, s);
   attractMode.setDependencies(game, machineOps, s);

   game.setScore(0, TRIDENT2020_MAJOR_VERSION);
   game.setCurrentPlayerScore(TRIDENT2020_MAJOR_VERSION);
   game.setScore(1, TRIDENT2020_MINOR_VERSION);
   game.setScore(2, RPU_OS_MAJOR_VERSION);
   game.setScore(3, RPU_OS_MINOR_VERSION);
   s.resetScoresToClearVersion = true;

   attractMode.enter(CurrentTime);
}

void loop() {
   RPU_DataRead(0);
   CurrentTime = millis();

   // Tick audio handlers first so currentTime_ is fresh when game logic calls playSoundEffect.
   machineOps.update(CurrentTime);

   TopState newState = activeMode->update(CurrentTime);
   if (newState != topState) {
      activeMode->exit();
      topState = newState;
      switch (topState) {
      case TopState::SelfTest: activeMode = &selfTestMode; break;
      case TopState::Attract:  activeMode = &attractMode;  break;
      case TopState::Game:     activeMode = &game;         break;
      }
      activeMode->enter(CurrentTime);
   }

   RPU_Update(CurrentTime);
}
