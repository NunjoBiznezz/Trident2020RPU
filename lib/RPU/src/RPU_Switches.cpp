/**************************************************************************
 *     This file is part of the RPU for Arduino Project.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 3/31/2023

    RPU is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPU is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#include "CircularQueue.h"
#include "RPU.h"
#include "RPU_Addresses.h"
#include "RPU_Solenoids.h"
#include "RPU_Switches.h"
#include "RPU_config.h"
#include <Arduino.h>

/******************************************************
 *   Switch State
 */
static int NumGameSwitches = 0;
static int NumGamePrioritySwitches = 0;
static const PlayfieldAndCabinetSwitch* GameSwitches = nullptr;

volatile uint8_t SwitchesMinus2[NUM_SWITCH_BYTES];
volatile uint8_t SwitchesMinus1[NUM_SWITCH_BYTES];
volatile uint8_t SwitchesNow[NUM_SWITCH_BYTES];

class SwitchStackClass : public CircularQueue<uint8_t, 60, 0xff> {
public:
   bool push(uint8_t data) {
      // Self test is a special case — if it's already first on the stack, ignore it
      if (data == SW_SELF_TEST_SWITCH) {
         if (!isEmpty() && peek() == SW_SELF_TEST_SWITCH) {
            return false;
         }
      }
      return CircularQueue<uint8_t, 60, 0xff>::push(data);
   }
};

static SwitchStackClass SwitchStack;

void RPU_ResetSwitchState() {
   SwitchStack.reset();
   for (uint8_t switchCount = 0; switchCount < NUM_SWITCH_BYTES; switchCount++) {
      SwitchesMinus2[switchCount] = 0xFF;
      SwitchesMinus1[switchCount] = 0xFF;
      SwitchesNow[switchCount] = 0xFF;
   }
}

/******************************************************
 *   Switch Handling Functions
 */

void RPU_PushToSwitchStack(uint8_t switchNumber) {
   SwitchStack.push(switchNumber);
}

uint8_t RPU_PullFirstFromSwitchStack() {
   return SwitchStack.pull();
}

bool RPU_ReadSingleSwitchState(uint8_t switchNum) {
   if (switchNum >= MAX_NUM_SWITCHES) {
      return false;
   }
   int switchByte = switchNum / 8;
   int switchBit = switchNum % 8;
   if (((SwitchesNow[switchByte]) >> switchBit) & 0x01) {
      return true;
   } else {
      return false;
   }
}

void RPU_SetupGameSwitches(int s_numSwitches, int s_numPrioritySwitches, const PlayfieldAndCabinetSwitch* s_gameSwitchArray) {
   NumGameSwitches = s_numSwitches;
   NumGamePrioritySwitches = s_numPrioritySwitches;
   GameSwitches = s_gameSwitchArray;
}

void RPU_ClearUpDownSwitchState() {
   // Bally/Stern does not have an up/down switch
}

bool RPU_GetUpDownSwitchState() {
   // Bally/Stern does not have an up/down switch
   return true;
}

/******************************************************
 *   Switch ISR Service (called from zero-crossing ISR in RPU.cpp)
 *
 *   Reads one bank of switches, debounces over three samples, and
 *   fires immediate solenoids and pushes validated closures to the
 *   game switch stack. Called once per bank per zero-crossing tick.
 */
void RPU_ServiceSwitchBank(uint8_t switchCount) {
   SwitchesMinus2[switchCount] = SwitchesMinus1[switchCount];
   SwitchesMinus1[switchCount] = SwitchesNow[switchCount];

   // Enable switch strobe for this bank
   RPU_DataWrite(ADDRESS_U10_A, 0x01 << switchCount);
   // Turn off U10:CB2 (strobes the last bank of DIP switches)
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x34);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   SwitchesNow[switchCount] = RPU_DataRead(ADDRESS_U10_B);
   RPU_DataWrite(ADDRESS_U10_A, 0x00);

   // Starting closures (off→on): trigger immediate solenoids
   uint8_t startingClosures = SwitchesNow[switchCount] & (~SwitchesMinus1[switchCount]);
   bool immediateSolenoidFired = false;
   if (startingClosures != 0) {
      for (uint8_t bitCount = 0; bitCount < 8 && !immediateSolenoidFired; bitCount++) {
         if ((startingClosures & 0x01) != 0) {
            uint8_t startingSwitchNum = switchCount * 8 + bitCount;
            for (int immediateSwitchCount = 0; immediateSwitchCount < NumGamePrioritySwitches && !immediateSolenoidFired;
                 immediateSwitchCount++) {
               if ((GameSwitches != nullptr) && (startingSwitchNum == GameSwitches[immediateSwitchCount].switchNum)) {
                  PushToFrontOfSolenoidStack(GameSwitches[immediateSwitchCount].solenoid, 1);
                  immediateSolenoidFired = true;
               }
            }
         }
         startingClosures = startingClosures >> 1;
      }
   }

   // Valid closures (off→on→on): push solenoids and notify game rules
   immediateSolenoidFired = false;
   uint8_t validClosures = (SwitchesNow[switchCount] & SwitchesMinus1[switchCount]) & ~SwitchesMinus2[switchCount];
   if (validClosures != 0) {
      for (uint8_t bitCount = 0; bitCount < 8; bitCount++) {
         if ((validClosures & 0x01) != 0) {
            uint8_t validSwitchNum = switchCount * 8 + bitCount;
            for (int validSwitchCount = 0; validSwitchCount < NumGameSwitches; validSwitchCount++) {
               if (GameSwitches != nullptr && GameSwitches[validSwitchCount].switchNum == validSwitchNum) {
                  if (GameSwitches[validSwitchCount].solenoid != SOL_NONE) {
                     if (validSwitchCount < NumGamePrioritySwitches && !immediateSolenoidFired) {
                        PushToFrontOfSolenoidStack(GameSwitches[validSwitchCount].solenoid,
                                                   GameSwitches[validSwitchCount].solenoidHoldTime);
                     } else {
                        RPU_PushToSolenoidStack(GameSwitches[validSwitchCount].solenoid,
                                                GameSwitches[validSwitchCount].solenoidHoldTime);
                     }
                  }
               }
            }
            SwitchStack.push(validSwitchNum);
         }
         validClosures = validClosures >> 1;
      }
   }

   // Allow display interrupt to fire between banks, then pad timing
   interrupts();
   delayMicroseconds(RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS);
   noInterrupts();
}
