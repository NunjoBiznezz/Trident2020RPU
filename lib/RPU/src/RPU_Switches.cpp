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

#include "RPU_Addresses.h"
#include "RPU_Solenoids.h"
#include "RPU_Switches.h"
#include <Arduino.h>

SwitchManager switches;

void SwitchManager::reset() {
   switchStack_.reset();
   for (uint8_t i = 0; i < NUM_SWITCH_BYTES; i++) {
      minus2_[i] = 0xFF;
      minus1_[i] = 0xFF;
      now_[i] = 0xFF;
   }
}

void SwitchManager::setup(int numSwitches, int numPrioritySwitches, const PlayfieldAndCabinetSwitch* switchArray) {
   numGameSwitches_ = numSwitches;
   numPrioritySwitches_ = numPrioritySwitches;
   gameSwitches_ = switchArray;
}

void SwitchManager::pushToStack(uint8_t switchNumber) {
   if (switchNumber == SW_SELF_TEST_SWITCH && !switchStack_.isEmpty() && switchStack_.peek() == SW_SELF_TEST_SWITCH) {
      return;
   }
   switchStack_.push(switchNumber);
}

uint8_t SwitchManager::pullFromStack() {
   return switchStack_.pull();
}

bool SwitchManager::readState(uint8_t switchNum) const {
   if (switchNum >= MAX_NUM_SWITCHES) {
      return false;
   }
   int switchByte = switchNum / 8;
   int switchBit = switchNum % 8;
   return ((now_[switchByte] >> switchBit) & 0x01) != 0;
}

void SwitchManager::clearUpDown() {
   // Bally/Stern does not have an up/down switch
}

bool SwitchManager::getUpDown() const {
   // Bally/Stern does not have an up/down switch
   return true;
}

void SwitchManager::serviceBank(uint8_t switchCount) {
   minus2_[switchCount] = minus1_[switchCount];
   minus1_[switchCount] = now_[switchCount];

   RPU_DataWrite(ADDRESS_U10_A, 0x01 << switchCount);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x34);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   now_[switchCount] = RPU_DataRead(ADDRESS_U10_B);
   RPU_DataWrite(ADDRESS_U10_A, 0x00);

   // Starting closures (off→on): fire immediate solenoids
   uint8_t startingClosures = now_[switchCount] & (~minus1_[switchCount]);
   bool immediateSolenoidFired = false;
   if (startingClosures != 0) {
      for (uint8_t bitCount = 0; bitCount < 8 && !immediateSolenoidFired; bitCount++) {
         if ((startingClosures & 0x01) != 0) {
            uint8_t switchNum = switchCount * 8 + bitCount;
            for (int i = 0; i < numPrioritySwitches_ && !immediateSolenoidFired; i++) {
               if (gameSwitches_ != nullptr && switchNum == gameSwitches_[i].switchNum) {
                  solenoids.pushToFront(gameSwitches_[i].solenoid, 1);
                  immediateSolenoidFired = true;
               }
            }
         }
         startingClosures >>= 1;
      }
   }

   // Valid closures (off→on→on): push solenoids and notify game rules
   immediateSolenoidFired = false;
   uint8_t validClosures = (now_[switchCount] & minus1_[switchCount]) & ~minus2_[switchCount];
   if (validClosures != 0) {
      for (uint8_t bitCount = 0; bitCount < 8; bitCount++) {
         if ((validClosures & 0x01) != 0) {
            uint8_t switchNum = switchCount * 8 + bitCount;
            for (int i = 0; i < numGameSwitches_; i++) {
               if (gameSwitches_ != nullptr && gameSwitches_[i].switchNum == switchNum) {
                  if (gameSwitches_[i].solenoid != SOL_NONE) {
                     if (i < numPrioritySwitches_ && !immediateSolenoidFired) {
                        solenoids.pushToFront(gameSwitches_[i].solenoid, gameSwitches_[i].solenoidHoldTime);
                     } else {
                        solenoids.push(gameSwitches_[i].solenoid, gameSwitches_[i].solenoidHoldTime);
                     }
                  }
               }
            }
            switchStack_.push(switchNum);
         }
         validClosures >>= 1;
      }
   }

   interrupts();
   delayMicroseconds(RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS);
   noInterrupts();
}

/******************************************************
 *   Public API
 */

void RPU_PushToSwitchStack(uint8_t switchNumber) {
   switches.pushToStack(switchNumber);
}

uint8_t RPU_PullFirstFromSwitchStack() {
   return switches.pullFromStack();
}

bool RPU_ReadSingleSwitchState(uint8_t switchNum) {
   return switches.readState(switchNum);
}

void RPU_SetupGameSwitches(int s_numSwitches, int s_numPrioritySwitches, const PlayfieldAndCabinetSwitch* s_gameSwitchArray) {
   switches.setup(s_numSwitches, s_numPrioritySwitches, s_gameSwitchArray);
}

void RPU_ClearUpDownSwitchState() {
   switches.clearUpDown();
}

bool RPU_GetUpDownSwitchState() {
   return switches.getUpDown();
}
