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
#include "RPU_config.h"
#include "RPU_Internal.h"
#include "RPU_Display.h"
#include <Arduino.h>

#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
constexpr uint8_t RPU_OS_MASK_SHIFT_1 = 0x60;
constexpr uint8_t RPU_OS_MASK_SHIFT_2 = 0x0C;
#else
constexpr uint8_t RPU_OS_MASK_SHIFT_1 = 0x30;
constexpr uint8_t RPU_OS_MASK_SHIFT_2 = 0x06;
#endif

DisplayManager displays;

void DisplayManager::reset() {
   currentDigit_ = 0;
   for (int d = 0; d < 5; d++) {
      for (unsigned dig = 0; dig < RPU_OS_NUM_DIGITS; dig++) {
         digits_[d][dig] = 0;
      }
      digitEnable_[d] = 0x00;
   }
}

uint8_t DisplayManager::setDisplay(int displayNumber, unsigned long value, bool blankByMagnitude, uint8_t minDigits,
                                   bool showCommasByMagnitude) {
   if (displayNumber < 0 || displayNumber > 4) {
      return 0;
   }

   uint8_t blank = 0x00;
   for (unsigned count = 0; count < RPU_OS_NUM_DIGITS; count++) {
      blank = blank * 2;
      if (value != 0 || count < minDigits) {
         blank |= 1;
      }
      (void)showCommasByMagnitude;
      digits_[displayNumber][(RPU_OS_NUM_DIGITS - 1) - count] = value % 10;
      value /= 10;
   }

   if (blankByMagnitude) {
      digitEnable_[displayNumber] = blank;
   }
   return blank;
}

void DisplayManager::setCredits(int value, bool displayOn, bool showBothDigits) {
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
   digits_[4][2] = (value % 100) / 10;
   digits_[4][3] = (value % 10);
#else
   digits_[4][1] = (value % 100) / 10;
   digits_[4][2] = (value % 10);
#endif
   uint8_t enableMask = digitEnable_[4] & RPU_OS_MASK_SHIFT_1;

   if (displayOn) {
      if (value > 9 || showBothDigits) {
         enableMask |= RPU_OS_MASK_SHIFT_2;
      } else {
         enableMask |= 0x04;
      }
   }
   digitEnable_[4] = enableMask;
}

void DisplayManager::setBallInPlay(int value, bool displayOn, bool showBothDigits) {
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
   digits_[4][5] = (value % 100) / 10;
   digits_[4][6] = (value % 10);
#else
   digits_[4][4] = (value % 100) / 10;
   digits_[4][5] = (value % 10);
#endif
   uint8_t enableMask = digitEnable_[4] & RPU_OS_MASK_SHIFT_2;

   if (displayOn) {
      if (value > 9 || showBothDigits) {
         enableMask |= RPU_OS_MASK_SHIFT_1;
      } else {
         enableMask |= 0x20;
      }
   }
   digitEnable_[4] = enableMask;
}

void DisplayManager::cycleAll(unsigned long curTime, uint8_t digitNum) {
   int displayDigit = (curTime / 250) % 10;
   unsigned long value;
#if (RPU_OS_NUM_DIGITS == 7)
   value = displayDigit * 1111111;
#else
   value = displayDigit * 111111;
#endif

   uint8_t displayNumToShow = 0;
   uint8_t displayBlank = RPU_OS_ALL_DIGITS_MASK;

   if (digitNum != 0) {
#if (RPU_OS_NUM_DIGITS == 7)
      displayNumToShow = (digitNum - 1) / 7;
      displayBlank = (0x40) >> ((digitNum - 1) % 7);
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
      if (displayNumToShow == 4) {
         displayBlank = (0x20) >> ((digitNum - 1) % 6);
      }
#endif
#else
      displayNumToShow = (digitNum - 1) / 6;
      displayBlank = (0x20) >> ((digitNum - 1) % 6);
#endif
   }

   for (int count = 0; count < 5; count++) {
      if (digitNum != 0) {
         setDisplay(count, value);
         setBlank(count, count == displayNumToShow ? displayBlank : 0);
      } else {
         setDisplay(count, value, false);
      }
   }
}

void DisplayManager::setMatch(int value, bool displayOn, bool showBothDigits) {
   setBallInPlay(value, displayOn, showBothDigits);
}

void DisplayManager::setBlank(int displayNumber, uint8_t bitMask) {
   if (displayNumber < 0 || displayNumber > 4) {
      return;
   }
   digitEnable_[displayNumber] = bitMask;
}

uint8_t DisplayManager::getBlank(int displayNumber) const {
   if (displayNumber < 0 || displayNumber > 4) {
      return 0;
   }
   return digitEnable_[displayNumber];
}

void DisplayManager::setFlash(int displayNumber, unsigned long value, unsigned long curTime, unsigned period, uint8_t minDigits) {
   if (period != 0) {
      if ((curTime / period) % 2 != 0) {
         setDisplay(displayNumber, value, true, minDigits);
      } else {
         setBlank(displayNumber, 0);
      }
   }
}

void DisplayManager::setFlashCredits(unsigned long curTime, int period) {
   if (period != 0) {
      if ((curTime / period) % 2 != 0) {
         digitEnable_[4] |= 0x06;
      } else {
         digitEnable_[4] &= 0x39;
      }
   }
}

/******************************************************
 *   Display ISR (Rev 1/2 hardware, ~320 Hz via TIMER1 CTC)
 *
 *   Multiplexes all 5 displays one digit at a time, latching BCD
 *   data and digit-enable signals through the U10/U11 PIAs.
 */
void DisplayManager::serviceISR() {
   uint8_t backupU10A = RPU_DataRead(ADDRESS_U10_A);

   // Disable lamp decoders & strobe latch
   RPU_DataWrite(ADDRESS_U10_A, 0xFF);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);
#ifdef RPU_OS_USE_AUX_LAMPS
   RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
   RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
#endif

   // Blank displays
   RPU_DataWrite(ADDRESS_U10_A_CONTROL, RPU_DataRead(ADDRESS_U10_A_CONTROL) & 0xF7);
   RPU_DataWrite(ADDRESS_U11_A, (RPU_DataRead(ADDRESS_U11_A)) | 0x01);
   RPU_DataWrite(ADDRESS_U10_A, 0x0F);

   uint8_t displayStrobeMask = 0x01;
   uint8_t displayDigitsMask;
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
   displayDigitsMask = (0x02 << currentDigit_);
#else
   displayDigitsMask = RPU_DataRead(ADDRESS_U11_A) & 0x02;
   displayDigitsMask |= (0x04 << currentDigit_);
#endif

   for (int d = 0; d < 5; d++) {
      uint8_t dataByte = (digits_[d][currentDigit_] << 4) | 0x0F;
      uint8_t enable = (digitEnable_[d] >> currentDigit_) & 0x01;
      if (!enable) {
         dataByte = 0xFF;
      }
      if (d < 4) {
         dataByte &= ~displayStrobeMask;
      }
      RPU_DataWrite(ADDRESS_U10_A, dataByte);
      if (d == 4) {
         RPU_DataWrite(ADDRESS_U11_A, displayDigitsMask & 0xFE);
      }
      delayMicroseconds(16);
      if (d < 4) {
         RPU_DataWrite(ADDRESS_U10_A, dataByte | 0x0F);
      } else {
         RPU_DataWrite(ADDRESS_U11_A, displayDigitsMask | 0x01);
      }
      displayStrobeMask *= 2;
   }

   RPU_DataWrite(ADDRESS_U11_A, displayDigitsMask | 0x01);

   currentDigit_++;
   if (currentDigit_ >= RPU_OS_NUM_DIGITS) {
      currentDigit_ = 0;
      offCycle_ ^= true;
   }

   RPU_DataWrite(ADDRESS_U10_A_CONTROL, RPU_DataRead(ADDRESS_U10_A_CONTROL) | 0x08);
   RPU_DataWrite(ADDRESS_U10_A, backupU10A);
}

/******************************************************
 *   Public API
 */

uint8_t RPU_SetDisplay(int displayNumber, unsigned long value, bool blankByMagnitude, uint8_t minDigits, bool showCommasByMagnitude) {
   return displays.setDisplay(displayNumber, value, blankByMagnitude, minDigits, showCommasByMagnitude);
}

void RPU_SetDisplayCredits(int value, bool displayOn, bool showBothDigits) {
   displays.setCredits(value, displayOn, showBothDigits);
}

void RPU_SetDisplayBallInPlay(int value, bool displayOn, bool showBothDigits) {
   displays.setBallInPlay(value, displayOn, showBothDigits);
}

void RPU_CycleAllDisplays(unsigned long curTime, uint8_t digitNum) {
   displays.cycleAll(curTime, digitNum);
}

void RPU_SetDisplayMatch(int value, bool displayOn, bool showBothDigits) {
   displays.setMatch(value, displayOn, showBothDigits);
}

void RPU_SetDisplayBlank(int displayNumber, uint8_t bitMask) {
   displays.setBlank(displayNumber, bitMask);
}

uint8_t RPU_GetDisplayBlank(int displayNumber) {
   return displays.getBlank(displayNumber);
}

void RPU_SetDisplayFlash(int displayNumber, unsigned long value, unsigned long curTime, unsigned period, uint8_t minDigits) {
   displays.setFlash(displayNumber, value, curTime, period, minDigits);
}

void RPU_SetDisplayFlashCredits(unsigned long curTime, int period) {
   displays.setFlashCredits(curTime, period);
}
