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
#include "adjustments/StoredAdjustmentsMode.h"
#include "adjustments/Trident2020AdjustmentsMode.h"
#include "adjustments/TridentAdjustmentsMode.h"
#include "AttractMode.h"
#include "BuildVersion.h"
#include "HardwareTestMode.h"
#include "MachineMode.h"
#include "MachineSettings.h"
#include "MatchMode.h"
#include "PinballMachine.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include "Trident2020Game.h"
#include "TridentGame.h"
#include <Arduino.h>
#include <stdint.h>

// Queryable build record embedded in flash. Extract with:
//   pio run -t version -e <env>
struct __attribute__((packed)) BuildInfoRecord {
   char magic[8]; // "TRID2020"
   uint16_t major;
   uint16_t minor;
   char branch[32];
   char describe[64];
   char built[24];
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
#define DEBUG_MESSAGE(x) DEBUG_PORT.write(x)
#else
#define DEBUG_MESSAGE(x)
#endif

/*********************************************************************
    Top-level objects — settings live inside pinballMachine
*********************************************************************/
static unsigned long CurrentTime = 0;

static PinballMachine pinballMachine;
static AttractMode attractMode;
static TridentGame tridentGame;
static Trident2020Game trident2020Game;
static MatchMode matchMode;
static HardwareTestMode hardwareTestMode;
static StoredAdjustmentsMode storedAdjustmentsMode;
static TridentAdjustmentsMode tridentAdjustmentsMode;
static Trident2020AdjustmentsMode t2020AdjustmentsMode;

static TopState topState = TopState::Attract;
static MachineMode* activeMode = &attractMode;

void setup() {
   // Opaque to LTO — prevents --gc-sections from discarding FIRMWARE_BUILD_INFO.
   __asm__ volatile("" ::"r"(&FIRMWARE_BUILD_INFO) : "memory");

#if defined(DEBUG_PORT)
   DEBUG_PORT.begin(115200);
#if defined(DEBUG_MESSAGES)
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
#endif
#endif

   CurrentTime = millis();
   pinballMachine.init(CurrentTime); // hardware setup, RPU init, diag notifications
   pinballMachine.readStoredParameters();

   trident2020Game.setMachine(pinballMachine);
   tridentGame.setMachine(pinballMachine);

   hardwareTestMode.setDependencies(pinballMachine);
   storedAdjustmentsMode.setDependencies(pinballMachine);
   tridentAdjustmentsMode.setDependencies(tridentGame, pinballMachine);
   t2020AdjustmentsMode.setDependencies(trident2020Game, pinballMachine);
   matchMode.setDependencies(pinballMachine);
   attractMode.setDependencies(pinballMachine);

   attractMode.enter(CurrentTime);
}

void loop() {
   pinballMachine.readInputs();
   CurrentTime = millis();

   // Tick audio handlers first so currentTime_ is fresh when trident2020Game logic calls playSoundEffect.
   pinballMachine.update(CurrentTime);

   TopState newState = activeMode->update(CurrentTime);
   if (newState != topState) {
      activeMode->exit();
      topState = newState;
      switch (topState) {
      case TopState::HardwareTest:
         activeMode = &hardwareTestMode;
         break;
      case TopState::MachineEeprom:
         activeMode = &storedAdjustmentsMode;
         break;
      case TopState::StoredAdjustments:
         activeMode = &storedAdjustmentsMode;
         break;
      case TopState::TridentAdjustments:
         activeMode = &tridentAdjustmentsMode;
         break;
      case TopState::Trident2020Adjustments:
         activeMode = &t2020AdjustmentsMode;
         break;
      case TopState::Attract:
         activeMode = &attractMode;
         break;
      case TopState::Match:
         activeMode = &matchMode;
         break;
      case TopState::Game:
         if (pinballMachine.getSettings().activeRuleSet == RuleSet::Original) {
            activeMode = &tridentGame;
         } else {
            activeMode = &trident2020Game;
         }
         break;
      }
      activeMode->enter(CurrentTime);
   }

   pinballMachine.flushOutputs();
}
