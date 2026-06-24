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
#include "RPU_DipSwitches.h"
#include <Arduino.h>

/******************************************************
 *   DIP Switch State
 */
#ifdef RPU_OS_USE_DIP_SWITCHES
static uint8_t DipSwitches[4];
#endif

/******************************************************
 *   DIP Switch Functions
 */

void RPU_ReadDipSwitches() {
#ifdef RPU_OS_USE_DIP_SWITCHES
   uint8_t backupU10A = RPU_DataRead(ADDRESS_U10_A);
   uint8_t backupU10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);

   // Turn on Switch strobe 5 & Read Switches
   RPU_DataWrite(ADDRESS_U10_A, 0x20);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   DipSwitches[0] = RPU_DataRead(ADDRESS_U10_B);

   // Turn on Switch strobe 6 & Read Switches
   RPU_DataWrite(ADDRESS_U10_A, 0x40);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   DipSwitches[1] = RPU_DataRead(ADDRESS_U10_B);

   // Turn on Switch strobe 7 & Read Switches
   RPU_DataWrite(ADDRESS_U10_A, 0x80);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   DipSwitches[2] = RPU_DataRead(ADDRESS_U10_B);

   // Turn on U10 CB2 (strobe 8) and read switches
   RPU_DataWrite(ADDRESS_U10_A, 0x00);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl | 0x08);
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   DipSwitches[3] = RPU_DataRead(ADDRESS_U10_B);

   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl);
   RPU_DataWrite(ADDRESS_U10_A, backupU10A);
#endif
}

uint8_t RPU_GetDipSwitches(uint8_t index) {
#ifdef RPU_OS_USE_DIP_SWITCHES
   if (index > 3) {
      return 0x00;
   }
   return DipSwitches[index];
#else
   return 0x00 & index;
#endif
}
