/**************************************************************************
 * EepromHelpers.h
 *
 * Inline EEPROM read helpers shared by PinballMachine, TridentGame, and
 * Trident2020Game.  Treats 0xFF / 0xFFFFFFFF as "uninitialized" and writes
 * the supplied default back to EEPROM on first read.
 **************************************************************************/

#pragma once
#include <EEPROM.h>
#include <stdint.h>

inline uint8_t readEepromSetting(int addr, uint8_t defaultVal = 0) {
   uint8_t v = EEPROM.read(addr);
   if (v == 0xFF) {
      EEPROM.write(addr, defaultVal);
      return defaultVal;
   }
   return v;
}

inline uint32_t readEepromULSetting(int addr, uint32_t defaultVal = 0) {
   uint32_t v;
   EEPROM.get(addr, v);
   if (v == 0xFFFFFFFF) {
      EEPROM.put(addr, defaultVal);
      return defaultVal;
   }
   return v;
}
