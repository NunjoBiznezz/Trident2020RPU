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
#include "RPU_Display.h"
#include <Arduino.h>

/******************************************************
 *   Display State
 */
static volatile uint8_t DisplayDigits[5][RPU_OS_NUM_DIGITS];
static volatile uint8_t DisplayDigitEnable[5];
static volatile bool DisplayOffCycle = false;
static volatile uint8_t CurrentDisplayDigit = 0;

void RPU_ResetDisplayState() {
   CurrentDisplayDigit = 0;
   for (int displayCount = 0; displayCount < 5; displayCount++) {
      for (int digitCount = 0; digitCount < RPU_OS_NUM_DIGITS; digitCount++) {
         DisplayDigits[displayCount][digitCount] = 0;
      }
      DisplayDigitEnable[displayCount] = 0x00;
   }
}

/******************************************************
 *   Display Handling Functions
 */
uint8_t RPU_SetDisplay(int displayNumber, unsigned long value, bool blankByMagnitude, uint8_t minDigits, bool showCommasByMagnitude) {
   if (displayNumber < 0 || displayNumber > 4) {
      return 0;
   }

   uint8_t blank = 0x00;

   for (int count = 0; count < RPU_OS_NUM_DIGITS; count++) {
      blank = blank * 2;
      if (value != 0 || count < minDigits) {
         blank |= 1;
      }

      (void)showCommasByMagnitude;

      DisplayDigits[displayNumber][(RPU_OS_NUM_DIGITS - 1) - count] = value % 10;
      value /= 10;
   }

   if (blankByMagnitude) {
      DisplayDigitEnable[displayNumber] = blank;
   }

   return blank;
}

void RPU_SetDisplayCredits(int value, bool displayOn, bool showBothDigits) {
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
   DisplayDigits[4][2] = (value % 100) / 10;
   DisplayDigits[4][3] = (value % 10);
#else
   DisplayDigits[4][1] = (value % 100) / 10;
   DisplayDigits[4][2] = (value % 10);
#endif
   uint8_t enableMask = DisplayDigitEnable[4] & RPU_OS_MASK_SHIFT_1;

   if (displayOn) {
      if (value > 9 || showBothDigits) {
         enableMask |= RPU_OS_MASK_SHIFT_2;
      } else {
         enableMask |= 0x04;
      }
   }

   DisplayDigitEnable[4] = enableMask;
}

void RPU_SetDisplayBallInPlay(int value, bool displayOn, bool showBothDigits) {
#ifdef RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS
   DisplayDigits[4][5] = (value % 100) / 10;
   DisplayDigits[4][6] = (value % 10);
#else
   DisplayDigits[4][4] = (value % 100) / 10;
   DisplayDigits[4][5] = (value % 10);
#endif
   uint8_t enableMask = DisplayDigitEnable[4] & RPU_OS_MASK_SHIFT_2;

   if (displayOn) {
      if (value > 9 || showBothDigits) {
         enableMask |= RPU_OS_MASK_SHIFT_1;
      } else {
         enableMask |= 0x20;
      }
   }

   DisplayDigitEnable[4] = enableMask;
}

void RPU_CycleAllDisplays(unsigned long curTime, uint8_t digitNum) {
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
         RPU_SetDisplay(count, value);
         if (count == displayNumToShow) {
            RPU_SetDisplayBlank(count, displayBlank);
         } else {
            RPU_SetDisplayBlank(count, 0);
         }
      } else {
         RPU_SetDisplay(count, value, false);
      }
   }
}

void RPU_SetDisplayMatch(int value, bool displayOn, bool showBothDigits) {
   RPU_SetDisplayBallInPlay(value, displayOn, showBothDigits);
}

// Digit mask layout (left to right on display):
//   digit=  1  2  3  4  5  6
//   bit=   b0 b1 b2 b3 b4 b5
void RPU_SetDisplayBlank(int displayNumber, uint8_t bitMask) {
   if (displayNumber < 0 || displayNumber > 4) {
      return;
   }

   DisplayDigitEnable[displayNumber] = bitMask;
}

uint8_t RPU_GetDisplayBlank(int displayNumber) {
   if (displayNumber < 0 || displayNumber > 4) {
      return 0;
   }
   return DisplayDigitEnable[displayNumber];
}

void RPU_SetDisplayFlash(int displayNumber, unsigned long value, unsigned long curTime, unsigned period, uint8_t minDigits) {
   if (period != 0) {
      if ((curTime / period) % 2 != 0) {
         RPU_SetDisplay(displayNumber, value, true, minDigits);
      } else {
         RPU_SetDisplayBlank(displayNumber, 0);
      }
   }
}

void RPU_SetDisplayFlashCredits(unsigned long curTime, int period) {
   if (period != 0) {
      if ((curTime / period) % 2 != 0) {
         DisplayDigitEnable[4] |= 0x06;
      } else {
         DisplayDigitEnable[4] &= 0x39;
      }
   }
}

/******************************************************
 *   Display Interrupt Service Routine (Rev 1/2 hardware)
 *
 *   Fires at ~320 Hz via TIMER1 CTC. Multiplexes all 5 displays
 *   one digit at a time, latching BCD data and digit-enable signals
 *   through the U10/U11 PIAs.
 */
ISR(TIMER1_COMPA_vect) {
   // Backup U10A
   uint8_t backupU10A = RPU_DataRead(ADDRESS_U10_A);

   // Disable lamp decoders & strobe latch
   RPU_DataWrite(ADDRESS_U10_A, 0xFF);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);
#ifdef RPU_OS_USE_AUX_LAMPS
   // Also park the aux lamp board
   RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
   RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
#endif

   // Blank Displays
   RPU_DataWrite(ADDRESS_U10_A_CONTROL, RPU_DataRead(ADDRESS_U10_A_CONTROL) & 0xF7);
   // Set all 5 display latch strobes high
   RPU_DataWrite(ADDRESS_U11_A, (RPU_DataRead(ADDRESS_U11_A)) | 0x01);
   RPU_DataWrite(ADDRESS_U10_A, 0x0F);

   uint8_t displayStrobeMask = 0x01;
   uint8_t displayDigitsMask;
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
   displayDigitsMask = (0x02 << CurrentDisplayDigit);
#else
   displayDigitsMask = RPU_DataRead(ADDRESS_U11_A) & 0x02;
   displayDigitsMask |= (0x04 << CurrentDisplayDigit);
#endif

   // Write current display digits to 5 displays
   for (int displayCount = 0; displayCount < 5; displayCount++) {
      // The BCD for this digit is in b4-b7, and the display latch strobes are in b0-b3 (and U11A:b0)
      uint8_t displayDataByte = ((DisplayDigits[displayCount][CurrentDisplayDigit]) << 4) | 0x0F;
      uint8_t displayEnable = ((DisplayDigitEnable[displayCount]) >> CurrentDisplayDigit) & 0x01;

      // if this digit shouldn't be displayed, then set data lines to 0xFX so digit will be blank
      if (!displayEnable) {
         displayDataByte = 0xFF;
      }

      // Calculate which bit needs to be dropped
      if (displayCount < 4) {
         displayDataByte &= ~(displayStrobeMask);
      }

      // Write out the digit & strobe (if it's 0-3)
      // The current number to display is the upper nibble of displayDataByte,
      // and the lower nibble is the strobe lines for the four score displays.
      // The strobe for the four score displays is high here because then the strobes
      // are NOR'd with U10:CA2 (which mutes the signals during other actions).
      // Only one strobe is low (from the above line.
      RPU_DataWrite(ADDRESS_U10_A, displayDataByte);
      if (displayCount == 4) {
         // Strobe #5 latch on U11A:b0
         RPU_DataWrite(ADDRESS_U11_A, displayDigitsMask & 0xFE);
      }

      // Right now the "Display Latch Strobe" is high

      // Put the latch strobe bits back high (low on the port)
      delayMicroseconds(16);

      if (displayCount < 4) {
         displayDataByte |= 0x0F;
         // Need to delay a little to make sure the strobe is low (high on the port) for long enough
         RPU_DataWrite(ADDRESS_U10_A, displayDataByte);
      } else {
         RPU_DataWrite(ADDRESS_U11_A, displayDigitsMask | 0x01);
      }

      displayStrobeMask *= 2;
   }

   // While the data is being strobed, we need to enable the current digit
   RPU_DataWrite(ADDRESS_U11_A, displayDigitsMask | 0x01);

   CurrentDisplayDigit = CurrentDisplayDigit + 1;
   if (CurrentDisplayDigit >= RPU_OS_NUM_DIGITS) {
      CurrentDisplayDigit = 0;
      DisplayOffCycle ^= true;
   }

   // Stop Blanking (current digits are all latched and ready)
   RPU_DataWrite(ADDRESS_U10_A_CONTROL, RPU_DataRead(ADDRESS_U10_A_CONTROL) | 0x08);

   // Restore 10A from backup
   RPU_DataWrite(ADDRESS_U10_A, backupU10A);
}
