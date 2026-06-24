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
#include "RPU_Solenoids.h"
#include <Arduino.h>

SolenoidManager solenoids;

void SolenoidManager::reset() {
   stack_.reset();
   timedStack_.reset();
}

void SolenoidManager::initDefault() {
   RPU_DataWrite(ADDRESS_U11_B, DEFAULT_SOLENOID_STATE);
   currentByte_ = DEFAULT_SOLENOID_STATE;
}

void SolenoidManager::writeContinuousByte() {
   RPU_DataWrite(ADDRESS_U11_B, currentByte_);
}

void SolenoidManager::push(uint8_t solenoidNumber, uint8_t numPushes, bool disableOverride) {
   if (solenoidNumber >= RPU_NUM_SOLENOIDS) {
      return;
   }
   if (!disableOverride && !stackEnabled_) {
      return;
   }
   for (int count = 0; count < numPushes; count++) {
      if (!stack_.push(solenoidNumber)) {
         return;
      }
   }
}

void SolenoidManager::pushToFront(uint8_t solenoidNumber, uint8_t numPushes) {
   if (!stackEnabled_) {
      return;
   }
   for (int count = 0; count < numPushes; count++) {
      if (!stack_.pushFront(solenoidNumber)) {
         return;
      }
   }
}

uint8_t SolenoidManager::pullFirst() {
   return stack_.pull();
}

bool SolenoidManager::pushTimed(uint8_t solenoidNumber, uint8_t numPushes, unsigned long whenToFire, bool disableOverride) {
   uint8_t slot = timedStack_.findFreeSlot();
   if (slot >= timedStack_.getSize()) {
      return false;
   }
   timedStack_.entries[slot].inUse = true;
   timedStack_.entries[slot].pushTime = whenToFire;
   timedStack_.entries[slot].disableOverride = disableOverride;
   timedStack_.entries[slot].solenoidNumber = solenoidNumber;
   timedStack_.entries[slot].numPushes = numPushes;
   return true;
}

void SolenoidManager::updateTimed(unsigned long curTime) {
   for (int count = 0; count < timedStack_.getSize(); count++) {
      if (timedStack_.entries[count].inUse && timedStack_.entries[count].pushTime < curTime) {
         push(timedStack_.entries[count].solenoidNumber, timedStack_.entries[count].numPushes,
              timedStack_.entries[count].disableOverride);
         timedStack_.entries[count].inUse = false;
      }
   }
}

void SolenoidManager::setCoinLockout(bool lockoutOff, uint8_t solbit) {
   if (!lockoutOff) {
      currentByte_ &= ~solbit;
   } else {
      currentByte_ |= solbit;
   }
   writeContinuousByte();
}

void SolenoidManager::setDisableFlippers(bool disableFlippers, uint8_t solbit) {
   if (disableFlippers) {
      currentByte_ |= solbit;
   } else {
      currentByte_ &= ~solbit;
   }
   writeContinuousByte();
}

void SolenoidManager::setContinuousBit(bool bitOn, uint8_t solbit) {
   if (bitOn) {
      currentByte_ |= solbit;
   } else {
      currentByte_ &= ~solbit;
   }
   writeContinuousByte();
}

bool SolenoidManager::fireContinuous(uint8_t solBit, uint8_t numCyclesToFire) {
   if (numCyclesBeforeRevert_ != 0) {
      return false;
   }
   numCyclesBeforeRevert_ = numCyclesToFire;
   revertBit_ = solBit;
   setContinuousBit(false, solBit);
   return true;
}

uint8_t SolenoidManager::readContinuous() const {
   return RPU_DataRead(ADDRESS_U11_B);
}

void SolenoidManager::enable() {
   stackEnabled_ = true;
}

void SolenoidManager::disable() {
   stackEnabled_ = false;
}

void SolenoidManager::service() {
   if (numCyclesBeforeRevert_ != 0) {
      numCyclesBeforeRevert_ -= 1;
      if (numCyclesBeforeRevert_ == 0) {
         currentByte_ |= revertBit_;
         revertBit_ = 0x00;
      }
   }

#ifdef RPU_OS_USE_DASH32
   uint8_t curDisplayDigitEnableByte = RPU_DataRead(ADDRESS_U11_A);
   RPU_DataWrite(ADDRESS_U11_A, curDisplayDigitEnableByte | 0x02);
#endif

   uint8_t momentarySolenoid = pullFirst();
   if (momentarySolenoid != SOLENOID_STACK_EMPTY) {
      currentByte_ = (currentByte_ & 0xF0) | momentarySolenoid;
      RPU_DataWrite(ADDRESS_U11_B, currentByte_);
#ifdef RPU_OS_USE_DASH32
      RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x3C);
      RPU_DataWrite(ADDRESS_U11_B, currentByte_ | SOL_NONE);
      RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);
      RPU_DataWrite(ADDRESS_U11_B, currentByte_);
#endif
   } else {
      currentByte_ = (currentByte_ & 0xF0) | SOL_NONE;
      RPU_DataWrite(ADDRESS_U11_B, currentByte_);
   }

#ifdef RPU_OS_USE_DASH32
   RPU_DataWrite(ADDRESS_U11_A, curDisplayDigitEnableByte);
#endif
}

/******************************************************
 *   Public API
 */

void RPU_PushToSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes, bool disableOverride) {
   solenoids.push(solenoidNumber, numPushes, disableOverride);
}

bool RPU_PushToTimedSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes, unsigned long whenToFire, bool disableOverride) {
   return solenoids.pushTimed(solenoidNumber, numPushes, whenToFire, disableOverride);
}

void RPU_UpdateTimedSolenoidStack(unsigned long curTime) {
   solenoids.updateTimed(curTime);
}

void RPU_SetCoinLockout(bool lockoutOff, uint8_t solbit) {
   solenoids.setCoinLockout(lockoutOff, solbit);
}

void RPU_SetDisableFlippers(bool disableFlippers, uint8_t solbit) {
   solenoids.setDisableFlippers(disableFlippers, solbit);
}

void RPU_SetContinuousSolenoidBit(bool bitOn, uint8_t solBit) {
   solenoids.setContinuousBit(bitOn, solBit);
}

bool RPU_FireContinuousSolenoid(uint8_t solBit, uint8_t numCyclesToFire) {
   return solenoids.fireContinuous(solBit, numCyclesToFire);
}

uint8_t RPU_ReadContinuousSolenoids() {
   return solenoids.readContinuous();
}

void RPU_DisableSolenoidStack() {
   solenoids.disable();
}

void RPU_EnableSolenoidStack() {
   solenoids.enable();
}
