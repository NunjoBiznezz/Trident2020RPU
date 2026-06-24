//
// Created by Douglas Bercot on 6/24/26.
//

#include "RPU_config.h"
#include "RPU_Core.h"
#include "RPU_Internal.h"
#include <Arduino.h>
#include <EEPROM.h>

/******************************************************
 *   EEPROM Helper Functions
 */

void RPU_WriteByteToEEProm(unsigned short startByte, uint8_t value) {
   EEPROM.write(startByte, value);
}

uint8_t RPU_ReadByteFromEEProm(unsigned short startByte) {
   uint8_t value = EEPROM.read(startByte);

   // If this value is unset, set it
   if (value == 0xFF) {
      value = 0;
      RPU_WriteByteToEEProm(startByte, value);
   }
   return value;
}

unsigned long RPU_ReadULFromEEProm(unsigned short startByte, unsigned long defaultValue) {
   unsigned long value;

   value = (((unsigned long)EEPROM.read(startByte + 3)) << 24) | ((unsigned long)(EEPROM.read(startByte + 2)) << 16) |
           ((unsigned long)(EEPROM.read(startByte + 1)) << 8) | ((unsigned long)(EEPROM.read(startByte)));

   if (value == 0xFFFFFFFF) {
      value = defaultValue;
      RPU_WriteULToEEProm(startByte, value);
   }
   return value;
}

void RPU_WriteULToEEProm(unsigned short startByte, unsigned long value) {
   EEPROM.write(startByte + 3, (uint8_t)(value >> 24));
   EEPROM.write(startByte + 2, (uint8_t)((value >> 16) & 0x000000FF));
   EEPROM.write(startByte + 1, (uint8_t)((value >> 8) & 0x000000FF));
   EEPROM.write(startByte, (uint8_t)(value & 0x000000FF));
}
