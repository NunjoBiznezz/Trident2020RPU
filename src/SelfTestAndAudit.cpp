/**************************************************************************
*     This file is part of the RPU OS for Arduino Project.

   I, Dick Hamill, the author of this program disclaim all copyright
   in order to make this program freely available in perpetuity to
   anyone who would like to use it. Dick Hamill, 6/1/2020

   RPU OS is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   RPU OS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   See <https://www.gnu.org/licenses/>.
*/

#include "SelfTestAndAudit.h"
#include "RPU.h"
#include "RPU_config.h"
#include "Trident.h"
#include <Arduino.h>

unsigned long LastSelfTestChange = 0;

#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE
bool CPCSelectionsHaveBeenRead = false;
const uint8_t CPCPairs[kNumCPCPairs][2] = {
   {1, 5}, {1, 4}, {1, 3}, {1, 2}, {1, 1},
   {2, 3}, {2, 1}, {3, 1}, {4, 1}
};
uint8_t CPCSelection[3];

uint8_t GetCPCSelection(uint8_t chuteNumber) {
   if (chuteNumber > 2) {
      return 0xFF;
   }

   if (!CPCSelectionsHaveBeenRead) {
      CPCSelection[0] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE);
      if (CPCSelection[0] >= kNumCPCPairs) {
         CPCSelection[0] = 4;
         RPU_WriteByteToEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE, 4);
      }
      CPCSelection[1] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE);
      if (CPCSelection[1] >= kNumCPCPairs) {
         CPCSelection[1] = 4;
         RPU_WriteByteToEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE, 4);
      }
      CPCSelection[2] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE);
      if (CPCSelection[2] >= kNumCPCPairs) {
         CPCSelection[2] = 4;
         RPU_WriteByteToEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE, 4);
      }
      CPCSelectionsHaveBeenRead = true;
   }

   return CPCSelection[chuteNumber];
}

uint8_t GetCPCCoins(uint8_t cpcSelection) {
   if (cpcSelection >= kNumCPCPairs) {
      return 1;
   }
   return CPCPairs[cpcSelection][0];
}

uint8_t GetCPCCredits(uint8_t cpcSelection) {
   if (cpcSelection >= kNumCPCPairs) {
      return 1;
   }
   return CPCPairs[cpcSelection][1];
}

void SetCPCSelection(uint8_t chuteNum, uint8_t value) {
   if (chuteNum < 3) {
      CPCSelection[chuteNum] = value;
   }
}
#endif

unsigned long GetLastSelfTestChangedTime() {
   return LastSelfTestChange;
}

void SetLastSelfTestChangedTime(unsigned long setSelfTestChange) {
   LastSelfTestChange = setSelfTestChange;
}
