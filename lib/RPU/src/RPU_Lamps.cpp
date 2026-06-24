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
#include "RPU_Lamps.h"
#include <Arduino.h>

/******************************************************
 *   Lamp State
 */
static volatile uint8_t LampStates[RPU_NUM_LAMP_BANKS];
static volatile uint8_t LampDim1[RPU_NUM_LAMP_BANKS];
static volatile uint8_t LampDim2[RPU_NUM_LAMP_BANKS];
static volatile uint8_t LampFlashPeriod[RPU_MAX_LAMPS];
static uint8_t DimDivisor1 = 2;
static uint8_t DimDivisor2 = 3;
static volatile int numberOfU10Interrupts = 0;

void RPU_ResetLampState() {
   for (int lampBankCounter = 0; lampBankCounter < RPU_NUM_LAMP_BANKS; lampBankCounter++) {
      LampStates[lampBankCounter] = 0xFF;
      LampDim1[lampBankCounter] = 0x00;
      LampDim2[lampBankCounter] = 0x00;
   }
   for (int lampFlashCount = 0; lampFlashCount < RPU_MAX_LAMPS; lampFlashCount++) {
      LampFlashPeriod[lampFlashCount] = 0;
   }
}

/******************************************************
 *   Lamp Handling Functions
 */

void RPU_SetDimDivisor(uint8_t level, uint8_t divisor) {
   if (level == 1) {
      DimDivisor1 = divisor;
   }
   if (level == 2) {
      DimDivisor2 = divisor;
   }
}

// left shift is iterative on Arduinos, so a bit array is surprisingly faster
static const uint8_t BitShiftValues[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

void RPU_SetLampState(int lampNum, bool lampState, uint8_t lampDim, int lampFlashPeriod) {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return;
   }
   uint8_t lampRow = lampNum % 8;
   uint8_t lampCol = lampNum / 8;
   uint8_t lampBit = BitShiftValues[lampRow];

   if (lampState) {
      int adjustedLampFlash = lampFlashPeriod / 50;

      if (lampFlashPeriod != 0 && adjustedLampFlash == 0) {
         adjustedLampFlash = 1;
      }
      if (adjustedLampFlash > 250) {
         adjustedLampFlash = 250;
      }

      // Only turn on the lamp if there's no flash, because if there's a flash
      // then the lamp will be turned on by the ApplyFlashToLamps function
      if (lampFlashPeriod == 0) {
         LampStates[lampCol] &= ~(lampBit);
      }
      LampFlashPeriod[lampNum] = adjustedLampFlash;
   } else {
      LampStates[lampCol] |= lampBit;
      LampFlashPeriod[lampNum] = 0;
   }

   if (lampDim & 0x01) {
      LampDim1[lampCol] |= lampBit;
   } else {
      LampDim1[lampCol] &= ~lampBit;
   }

   if (lampDim & 0x02) {
      LampDim2[lampCol] |= lampBit;
   } else {
      LampDim2[lampCol] &= ~lampBit;
   }
}

uint8_t RPU_ReadLampState(int lampNum) {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return 0x00;
   }
   uint8_t lampStateByte = LampStates[lampNum / 8];
   return (lampStateByte & (0x01 << (lampNum % 8))) ? 0 : 1;
}

uint8_t RPU_ReadLampDim(int lampNum) {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return 0x00;
   }
   uint8_t lampDim = 0;
   uint8_t lampDimByte = LampDim1[lampNum / 8];
   if ((lampDimByte & (0x01 << (lampNum % 8))) != 0) {
      lampDim |= 1;
   }

   lampDimByte = LampDim2[lampNum / 8];
   if ((lampDimByte & (0x01 << (lampNum % 8))) != 0) {
      lampDim |= 2;
   }

   return lampDim;
}

int RPU_ReadLampFlash(int lampNum) {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return 0;
   }
   return LampFlashPeriod[lampNum] * 50;
}

void RPU_ApplyFlashToLamps(unsigned long curTime) {
   int curLampByte = 0;
   uint8_t curLampBit = 0;
   int curLampNum = 0;

   for (curLampByte = 0; curLampByte < RPU_NUM_LAMP_BANKS; curLampByte++) {
      curLampBit = 0x01;
      for (uint8_t curBit = 0; curBit < 8; curBit++) {
         if (LampFlashPeriod[curLampNum] != 0) {
            unsigned long adjustedLampFlash = (unsigned long)LampFlashPeriod[curLampNum] * (unsigned long)50;
            if ((curTime / adjustedLampFlash) % 2 != 0) {
               LampStates[curLampByte] &= ~(curLampBit);
            } else {
               LampStates[curLampByte] |= (curLampBit);
            }
         }

         curLampBit *= 2;
         curLampNum += 1;
      }
   }
}

void RPU_FlashAllLamps(unsigned long curTime) {
   for (int count = 0; count < RPU_MAX_LAMPS; count++) {
      RPU_SetLampState(count, true, 0, 500);
   }
   RPU_ApplyFlashToLamps(curTime);
}

void RPU_TurnOffAllLamps() {
   for (int count = 0; count < RPU_MAX_LAMPS; count++) {
      RPU_SetLampState(count, false, 0, 0);
   }
}

/******************************************************
 *   Lamp Strobe (called from zero-crossing ISR in RPU.cpp)
 *
 *   Drives all lamp banks through the U10 PIA's multiplexed
 *   address/data lines, then parks the lamp board. Also manages
 *   the dim-divisor cycle counter used for two-level dimming.
 */
void RPU_StrobeLamps() {
   for (int lampByteCount = 0; lampByteCount < 8; lampByteCount++) {
      for (uint8_t nibbleCount = 0; nibbleCount < 2; nibbleCount++) {
         // We skip iteration number 16 because the last position is to park the lamps
         if (lampByteCount == 7 && nibbleCount != 0) {
            continue;
         }

         uint8_t lampData = 0xF0 + (lampByteCount * 2) + nibbleCount;

         interrupts();
         RPU_DataWrite(ADDRESS_U10_A, 0xFF);
         noInterrupts();

         // Latch address & strobe
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

         // Use the inhibit lines to set the actual data to the lamp SCRs
         // (here, we don't care about the lower nibble because the address was already latched)
         uint8_t nibbleOffset = (nibbleCount != 0) ? 1 : 16;
         uint8_t lampOutput = (LampStates[lampByteCount] * nibbleOffset);
         // Every other time through the cycle, we OR in the dim variable
         // in order to dim those lights
         if (numberOfU10Interrupts % DimDivisor1 != 0) {
            lampOutput |= (LampDim1[lampByteCount] * nibbleOffset);
         }
         if (numberOfU10Interrupts % DimDivisor2 != 0) {
            lampOutput |= (LampDim2[lampByteCount] * nibbleOffset);
         }

         RPU_DataWrite(ADDRESS_U10_A, lampOutput | 0x0F);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE
         delayMicroseconds(2);
#endif
      } // end loop on nibble
   }   // end loop on lamp bytes

#ifdef RPU_OS_USE_AUX_LAMPS
   // Latch 0xFF separately without interrupt clear
   // to park 0xFF in main lamp board
   RPU_DataWrite(ADDRESS_U10_A, 0xFF);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

   // For the first four bits of lamps, we're going to look at LampStates[7] again
   // and use those top 4 bits that we didn't use before. Then we're going
   // to move on with bytes 8, 9, and 10 for the remaining 24 bits of data
   uint8_t auxBankNum = 0;
   for (int lampByteCount = 7; lampByteCount < RPU_NUM_LAMP_BANKS; lampByteCount++) {
      for (uint8_t nibbleCount = 0; nibbleCount < 2; nibbleCount++) {
         if (lampByteCount == 7) {
            nibbleCount = 1; // skip the first nibble of uint8_t 7 because it belongs to primary lamps
         }
         uint8_t nibbleOffset = (nibbleCount != 0) ? 1 : 16;
         uint8_t lampOutput = (LampStates[lampByteCount] * nibbleOffset);
         // Every other time through the cycle, we OR in the dim variable
         // in order to dim those lights
         if (numberOfU10Interrupts % DimDivisor1 != 0) {
            lampOutput |= (LampDim1[lampByteCount] * nibbleOffset);
         }
         if (numberOfU10Interrupts % DimDivisor2 != 0) {
            lampOutput |= (LampDim2[lampByteCount] * nibbleOffset);
         }

         // The data will be in the upper nibble, but we need the bank count in the lower
         lampOutput &= 0xF0;
         lampOutput += auxBankNum;

         interrupts();
         RPU_DataWrite(ADDRESS_U10_A, 0xFF);
         noInterrupts();

         RPU_DataWrite(ADDRESS_U10_A, lampOutput | 0xF0);
         RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
         RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
         RPU_DataWrite(ADDRESS_U10_A, lampOutput);

         auxBankNum += 1;
      }
   }
#endif

   // Park the lamp board
   RPU_DataWrite(ADDRESS_U10_A, 0xFF);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

   numberOfU10Interrupts += 1;
}
