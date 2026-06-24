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

#include "RPU_Lamps.h"
#include "RPU_Core.h"
#include "RPU_Internal.h"
#include <Arduino.h>

LampManager lamps;

// left shift is iterative on Arduinos, so a bit array is faster
const uint8_t LampManager::BIT_SHIFT[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

void LampManager::reset() {
   for (int i = 0; i < RPU_NUM_LAMP_BANKS; i++) {
      states_[i] = 0xFF;
      dim1_[i] = 0x00;
      dim2_[i] = 0x00;
   }
   for (int i = 0; i < RPU_MAX_LAMPS; i++) {
      flashPeriod_[i] = 0;
   }
}

void LampManager::setDimDivisor(uint8_t level, uint8_t divisor) {
   if (level == 1) {
      dimDivisor1_ = divisor;
   } else if (level == 2) {
      dimDivisor2_ = divisor;
   }
}

void LampManager::setState(int lampNum, bool lampState, uint8_t lampDim, int lampFlashPeriod) {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return;
   }
   uint8_t lampBit = BIT_SHIFT[lampNum % 8];
   uint8_t lampCol = lampNum / 8;

   if (lampState) {
      int adjustedFlash = lampFlashPeriod / 50;
      if (lampFlashPeriod != 0 && adjustedFlash == 0) {
         adjustedFlash = 1;
      }
      if (adjustedFlash > 250) {
         adjustedFlash = 250;
      }
      if (lampFlashPeriod == 0) {
         states_[lampCol] &= ~lampBit;
      }
      flashPeriod_[lampNum] = adjustedFlash;
   } else {
      states_[lampCol] |= lampBit;
      flashPeriod_[lampNum] = 0;
   }

   if (lampDim & 0x01) {
      dim1_[lampCol] |= lampBit;
   } else {
      dim1_[lampCol] &= ~lampBit;
   }
   if (lampDim & 0x02) {
      dim2_[lampCol] |= lampBit;
   } else {
      dim2_[lampCol] &= ~lampBit;
   }
}

uint8_t LampManager::readState(int lampNum) const {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return 0x00;
   }
   uint8_t stateByte = states_[lampNum / 8];
   return (stateByte & (0x01 << (lampNum % 8))) ? 0 : 1;
}

uint8_t LampManager::readDim(int lampNum) const {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return 0x00;
   }
   uint8_t lampDim = 0;
   uint8_t shift = 0x01 << (lampNum % 8);
   if ((dim1_[lampNum / 8] & shift) != 0) {
      lampDim |= 1;
   }
   if ((dim2_[lampNum / 8] & shift) != 0) {
      lampDim |= 2;
   }
   return lampDim;
}

int LampManager::readFlash(int lampNum) const {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return 0;
   }
   return flashPeriod_[lampNum] * 50;
}

void LampManager::applyFlash(unsigned long curTime) {
   int curLampNum = 0;
   for (int bank = 0; bank < RPU_NUM_LAMP_BANKS; bank++) {
      uint8_t curBit = 0x01;
      for (uint8_t bit = 0; bit < 8; bit++) {
         if (flashPeriod_[curLampNum] != 0) {
            unsigned long period = (unsigned long)flashPeriod_[curLampNum] * 50UL;
            if ((curTime / period) % 2 != 0) {
               states_[bank] &= ~curBit;
            } else {
               states_[bank] |= curBit;
            }
         }
         curBit *= 2;
         curLampNum++;
      }
   }
}

void LampManager::flashAll(unsigned long curTime) {
   for (int i = 0; i < RPU_MAX_LAMPS; i++) {
      setState(i, true, 0, 500);
   }
   applyFlash(curTime);
}

void LampManager::turnOffAll() {
   for (int i = 0; i < RPU_MAX_LAMPS; i++) {
      setState(i, false, 0, 0);
   }
}

RPU_OPTIMIZE_SPEED void LampManager::strobe() {
   for (int bank = 0; bank < 8; bank++) {
      for (uint8_t nibble = 0; nibble < 2; nibble++) {
         // Skip the last position — used to park the lamp board
         if (bank == 7 && nibble != 0) {
            continue;
         }

         uint8_t lampData = 0xF0 + (bank * 2) + nibble;

         interrupts();
         RPU_DataWrite(ADDRESS_U10_A, 0xFF);
         noInterrupts();

         RPU_DataWrite(ADDRESS_U10_A, lampData);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE
         delayMicroseconds(2);
#endif
         RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x38);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE
         delayMicroseconds(2);
#endif
         RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x30);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE
         delayMicroseconds(2);
#endif

         uint8_t nibbleOffset = (nibble != 0) ? 1 : 16;
         uint8_t lampOutput = states_[bank] * nibbleOffset;
         if (numInterrupts_ % dimDivisor1_ != 0) {
            lampOutput |= dim1_[bank] * nibbleOffset;
         }
         if (numInterrupts_ % dimDivisor2_ != 0) {
            lampOutput |= dim2_[bank] * nibbleOffset;
         }

         RPU_DataWrite(ADDRESS_U10_A, lampOutput | 0x0F);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE
         delayMicroseconds(2);
#endif
      }
   }

#ifdef RPU_OS_USE_AUX_LAMPS
   // Park the primary lamp board before switching to aux
   RPU_DataWrite(ADDRESS_U10_A, 0xFF);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

   uint8_t auxBankNum = 0;
   for (int bank = 7; bank < RPU_NUM_LAMP_BANKS; bank++) {
      for (uint8_t nibble = 0; nibble < 2; nibble++) {
         if (bank == 7) {
            nibble = 1; // first nibble of bank 7 belongs to primary lamps
         }
         uint8_t nibbleOffset = (nibble != 0) ? 1 : 16;
         uint8_t lampOutput = states_[bank] * nibbleOffset;
         if (numInterrupts_ % dimDivisor1_ != 0) {
            lampOutput |= dim1_[bank] * nibbleOffset;
         }
         if (numInterrupts_ % dimDivisor2_ != 0) {
            lampOutput |= dim2_[bank] * nibbleOffset;
         }
         lampOutput = (lampOutput & 0xF0) + auxBankNum;

         interrupts();
         RPU_DataWrite(ADDRESS_U10_A, 0xFF);
         noInterrupts();

         RPU_DataWrite(ADDRESS_U10_A, lampOutput | 0xF0);
         RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
         RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
         RPU_DataWrite(ADDRESS_U10_A, lampOutput);

         auxBankNum++;
      }
   }
#endif

   // Park the lamp board
   RPU_DataWrite(ADDRESS_U10_A, 0xFF);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

   numInterrupts_++;
}

/******************************************************
 *   Public API
 */

void RPU_SetDimDivisor(uint8_t level, uint8_t divisor) {
   lamps.setDimDivisor(level, divisor);
}

void RPU_SetLampState(int lampNum, bool lampState, uint8_t lampDim, int lampFlashPeriod) {
   lamps.setState(lampNum, lampState, lampDim, lampFlashPeriod);
}

uint8_t RPU_ReadLampState(int lampNum) {
   return lamps.readState(lampNum);
}

uint8_t RPU_ReadLampDim(int lampNum) {
   return lamps.readDim(lampNum);
}

int RPU_ReadLampFlash(int lampNum) {
   return lamps.readFlash(lampNum);
}

void RPU_ApplyFlashToLamps(unsigned long curTime) {
   lamps.applyFlash(curTime);
}

void RPU_FlashAllLamps(unsigned long curTime) {
   lamps.flashAll(curTime);
}

void RPU_TurnOffAllLamps() {
   lamps.turnOffAll();
}
