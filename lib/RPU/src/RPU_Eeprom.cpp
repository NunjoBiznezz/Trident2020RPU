//
// Created by Douglas Bercot on 6/24/26.
//

#include "RPU.h"
#include <EEPROM.h>

/******************************************************
 *   EEPROM Helper Functions
 */

void RPU_WriteByteToEEProm(uint16_t startByte, uint8_t value) {
   EEPROM.write((int)startByte, value);
}

uint8_t RPU_ReadByteFromEEProm(uint16_t startByte, uint8_t defaultValue) {
   uint8_t value = EEPROM.read((int)startByte);

   // If this value is unset, set it
   if (value == 0xFF) {
      value = 0;
      RPU_WriteByteToEEProm(startByte, value);
   }
   return value;
}

uint16_t RPU_ReadShortFromEEProm(uint16_t startByte, uint16_t defaultValue) {
   uint16_t value =
      ((uint16_t)(EEPROM.read((int)startByte + 1)) << 8) |
      ((uint16_t)(EEPROM.read((int)startByte)));

   if (value == 0xFFFF) {
      value = defaultValue;
      RPU_WriteULToEEProm(startByte, value);
   }
   return value;
}

void RPU_WriteShortToEEProm(uint16_t startByte, uint16_t value) {
   EEPROM.write((int)startByte + 1, (uint8_t)((value >> 8) & 0x000000FF));
   EEPROM.write((int)startByte, (uint8_t)(value & 0x000000FF));
}

uint32_t RPU_ReadULFromEEProm(uint16_t startByte, uint32_t defaultValue) {
   uint32_t value = (((uint32_t)EEPROM.read((int)startByte + 3)) << 24) |
                     ((uint32_t)(EEPROM.read((int)startByte + 2)) << 16) |
                     ((uint32_t)(EEPROM.read((int)startByte + 1)) << 8) |
                     ((uint32_t)(EEPROM.read((int)startByte)));

   if (value == 0xFFFFFFFF) {
      value = defaultValue;
      RPU_WriteULToEEProm(startByte, value);
   }
   return value;
}

void RPU_WriteULToEEProm(uint16_t startByte, uint32_t value) {
   EEPROM.write(startByte + 3, (uint8_t)(value >> 24));
   EEPROM.write(startByte + 2, (uint8_t)((value >> 16) & 0x000000FF));
   EEPROM.write(startByte + 1, (uint8_t)((value >> 8) & 0x000000FF));
   EEPROM.write(startByte, (uint8_t)(value & 0x000000FF));
}
