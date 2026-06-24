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

#include "RPU.h"
#include "RPU_Addresses.h"
#include "RPU_config.h"
#include "RPU_Solenoids.h"
#include "TimedStack.h"
#include <Arduino.h>

/******************************************************
 *   Solenoid State
 */
#if (RPU_OS_HARDWARE_REV > 2)
#define SOLENOID_STACK_SIZE 150
#else
#define SOLENOID_STACK_SIZE 60
#endif

#include "CircularQueue.h"
static CircularQueue<uint8_t, SOLENOID_STACK_SIZE, SOLENOID_STACK_EMPTY> SolenoidStack;
static bool SolenoidStackEnabled = true;
static volatile uint8_t CurrentSolenoidByte = 0xFF;
static volatile uint8_t RevertSolenoidBit = 0x00;
static volatile uint8_t NumCyclesBeforeRevertingSolenoidByte = 0;

#define TIMED_SOLENOID_STACK_SIZE 30
static TimedStack<TimedSolenoidEntry, TIMED_SOLENOID_STACK_SIZE> TimedSolenoidStack;

void RPU_ResetSolenoidState() {
   SolenoidStack.reset();
   TimedSolenoidStack.reset();
}

void RPU_InitSolenoidDefault() {
   RPU_DataWrite(ADDRESS_U11_B, DEFAULT_SOLENOID_STATE);
   CurrentSolenoidByte = DEFAULT_SOLENOID_STATE;
}

/******************************************************
 *   Solenoid Handling Functions
 */

void RPU_PushToSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes, bool disableOverride) {
   if (solenoidNumber >= RPU_NUM_SOLENOIDS) {
      return;
   }

   if (!disableOverride && !SolenoidStackEnabled) {
      return;
   }

   for (int count = 0; count < numPushes; count++) {
      if (!SolenoidStack.push(solenoidNumber)) {
         return;
      }
   }
}

void PushToFrontOfSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes) {
   if (!SolenoidStackEnabled) {
      return;
   }

   for (int count = 0; count < numPushes; count++) {
      if (!SolenoidStack.pushFront(solenoidNumber)) {
         return;
      }
   }
}

uint8_t PullFirstFromSolenoidStack() {
   return SolenoidStack.pull();
}

bool RPU_PushToTimedSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes, unsigned long whenToFire, bool disableOverride) {
   uint8_t slot = TimedSolenoidStack.findFreeSlot();
   if (slot >= TimedSolenoidStack.getSize()) {
      return false;
   }
   TimedSolenoidStack.entries[slot].inUse = true;
   TimedSolenoidStack.entries[slot].pushTime = whenToFire;
   TimedSolenoidStack.entries[slot].disableOverride = disableOverride;
   TimedSolenoidStack.entries[slot].solenoidNumber = solenoidNumber;
   TimedSolenoidStack.entries[slot].numPushes = numPushes;
   return true;
}

void RPU_UpdateTimedSolenoidStack(unsigned long curTime) {
   for (int count = 0; count < TimedSolenoidStack.getSize(); count++) {
      if (TimedSolenoidStack.entries[count].inUse && TimedSolenoidStack.entries[count].pushTime < curTime) {
         RPU_PushToSolenoidStack(TimedSolenoidStack.entries[count].solenoidNumber, TimedSolenoidStack.entries[count].numPushes,
                                 TimedSolenoidStack.entries[count].disableOverride);
         TimedSolenoidStack.entries[count].inUse = false;
      }
   }
}

void RPU_SetCoinLockout(bool lockoutOff, uint8_t solbit) {
   if (!lockoutOff) {
      CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
   } else {
      CurrentSolenoidByte = CurrentSolenoidByte | solbit;
   }
   RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}

void RPU_SetDisableFlippers(bool disableFlippers, uint8_t solbit) {
   if (disableFlippers) {
      CurrentSolenoidByte = CurrentSolenoidByte | solbit;
   } else {
      CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
   }
   RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}

void RPU_SetContinuousSolenoidBit(bool bitOn, uint8_t solbit) {
   if (bitOn) {
      CurrentSolenoidByte = CurrentSolenoidByte | solbit;
   } else {
      CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
   }
   RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}

bool RPU_FireContinuousSolenoid(uint8_t solBit, uint8_t numCyclesToFire) {
   if (NumCyclesBeforeRevertingSolenoidByte != 0) {
      return false;
   }
   NumCyclesBeforeRevertingSolenoidByte = numCyclesToFire;
   RevertSolenoidBit = solBit;
   RPU_SetContinuousSolenoidBit(false, solBit);
   return true;
}

uint8_t RPU_ReadContinuousSolenoids() {
   return RPU_DataRead(ADDRESS_U11_B);
}

void RPU_DisableSolenoidStack() {
   SolenoidStackEnabled = false;
}

void RPU_EnableSolenoidStack() {
   SolenoidStackEnabled = true;
}

/******************************************************
 *   Solenoid ISR Service (called from zero-crossing ISR in RPU.cpp)
 *
 *   Manages the continuous-solenoid revert countdown, then pulls
 *   one entry from the stack and fires it via the U11B PIA.
 */
void RPU_ServiceSolenoids() {
   if (NumCyclesBeforeRevertingSolenoidByte != 0) {
      NumCyclesBeforeRevertingSolenoidByte -= 1;
      if (NumCyclesBeforeRevertingSolenoidByte == 0) {
         CurrentSolenoidByte |= RevertSolenoidBit;
         RevertSolenoidBit = 0x00;
      }
   }

#ifdef RPU_OS_USE_DASH32
   uint8_t curDisplayDigitEnableByte = RPU_DataRead(ADDRESS_U11_A);
   RPU_DataWrite(ADDRESS_U11_A, curDisplayDigitEnableByte | 0x02);
#endif

   uint8_t momentarySolenoidAtStart = PullFirstFromSolenoidStack();
   if (momentarySolenoidAtStart != SOLENOID_STACK_EMPTY) {
      CurrentSolenoidByte = (CurrentSolenoidByte & 0xF0) | momentarySolenoidAtStart;
      RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
#ifdef RPU_OS_USE_DASH32
      RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x3C);
      RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte | SOL_NONE);
      RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);
      RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
#endif
   } else {
      CurrentSolenoidByte = (CurrentSolenoidByte & 0xF0) | SOL_NONE;
      RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
   }

#ifdef RPU_OS_USE_DASH32
   RPU_DataWrite(ADDRESS_U11_A, curDisplayDigitEnableByte);
#endif
}
