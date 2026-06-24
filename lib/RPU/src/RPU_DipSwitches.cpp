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
#include "RPU_Internal.h"
#include "RPU_DipSwitches.h"
#include <Arduino.h>

DipSwitchManager dipSwitches;

void DipSwitchManager::read() {
#ifdef RPU_OS_USE_DIP_SWITCHES
   const uint8_t backupU10A = RPU_DataRead(ADDRESS_U10_A);
   const uint8_t backupU10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);

   RPU_DataWrite(ADDRESS_U10_A, 0x20);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   dipSwitches_[0] = RPU_DataRead(ADDRESS_U10_B);

   RPU_DataWrite(ADDRESS_U10_A, 0x40);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   dipSwitches_[1] = RPU_DataRead(ADDRESS_U10_B);

   RPU_DataWrite(ADDRESS_U10_A, 0x80);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   dipSwitches_[2] = RPU_DataRead(ADDRESS_U10_B);

   RPU_DataWrite(ADDRESS_U10_A, 0x00);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl | 0x08);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   dipSwitches_[3] = RPU_DataRead(ADDRESS_U10_B);

   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl);
   RPU_DataWrite(ADDRESS_U10_A, backupU10A);
#endif
}

uint8_t DipSwitchManager::get(uint8_t index) const {
#ifdef RPU_OS_USE_DIP_SWITCHES
   if (index > 3) {
      return 0x00;
   }
   return dipSwitches_[index];
#else
   return 0x00 & index;
#endif
}

/******************************************************
 *   Public API
 */

void RPU_ReadDipSwitches() {
   dipSwitches.read();
}

uint8_t RPU_GetDipSwitches(uint8_t index) {
   return dipSwitches.get(index);
}
