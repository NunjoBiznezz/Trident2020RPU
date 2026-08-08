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

#if RPU_MPU_ARCH_IS_BSOS()

#include "RPU_DipSwitches.h"
#include "RPU_Switches.h"
#include "RPU_Display.h"
#include "RPU_Lamps.h"
#include "RPU_Solenoids.h"
#include <Arduino.h>

/******************************************************
 *   PIA initialization (Bally/Stern architecture)
 */

static void InitializeU10PIA() {
   // CA1 - Self Test Switch
   // CB1 - zero crossing detector
   // CA2 - NOR'd with display latch zeroCrossingISR
   // CB2 - lamp zeroCrossingISR 1
   // PA0-7 - output for switch bank, lamps, and BCD
   // PB0-7 - switch returns

   RPU_DataWrite(ADDRESS_U10_A_CONTROL, 0x38);
   // Set up U10A as output
   RPU_DataWrite(ADDRESS_U10_A, 0xFF);
   // Set bit 3 to write data
   RPU_DataWrite(ADDRESS_U10_A_CONTROL, RPU_DataRead(ADDRESS_U10_A_CONTROL) | 0x04);
   // Store F0 in U10A Output
   RPU_DataWrite(ADDRESS_U10_A, 0xF0);

   RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x33);
   // Set up U10B as input
   RPU_DataWrite(ADDRESS_U10_B, 0x00);
   // Set bit 3 so future reads will read data
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x04);
}

static void InitializeU11PIA() {
   // CA1 - Display interrupt generator
   // CB1 - test connector pin 32
   // CA2 - lamp zeroCrossingISR 2
   // CB2 - solenoid bank select
   // PA0-7 - display digit enable
   // PB0-7 - solenoid data

   RPU_DataWrite(ADDRESS_U11_A_CONTROL, 0x31);
   // Set up U11A as output
   RPU_DataWrite(ADDRESS_U11_A, 0xFF);
   // Set bit 3 to write data
   RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x04);
   // Store 00 in U11A Output
   RPU_DataWrite(ADDRESS_U11_A, 0x00);

   RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x30);
   // Set up U11B as output
   RPU_DataWrite(ADDRESS_U11_B, 0xFF);
   // Set bit 3 so future reads will read data
   RPU_DataWrite(ADDRESS_U11_B_CONTROL, RPU_DataRead(ADDRESS_U11_B_CONTROL) | 0x04);

   // Store 9F in U11B Output and initialize solenoid state
   RPU::solenoids.initDefault();
}

/**
 * Once the BSP is initialized, verify that the PIA chips are responding correctly.
 * If the PIAs are not responding correctly, return a bitmask of errors.
 * @return A bitmask of PIA errors (RPU_RET_U10_PIA_ERROR, RPU_RET_U11_PIA_ERROR)
 */
static unsigned long RPU_TestPIAs() {
   unsigned long piaErrors = 0;

   uint8_t piaResult = RPU_DataRead(ADDRESS_U10_A_CONTROL);
   if (piaResult != 0x3C) {
      piaErrors |= RPU_RET_U10_PIA_ERROR;
   }
   piaResult = RPU_DataRead(ADDRESS_U10_B_CONTROL);
   if (piaResult != 0x37) {
      piaErrors |= RPU_RET_U10_PIA_ERROR;
   }

   piaResult = RPU_DataRead(ADDRESS_U11_A_CONTROL);
   if (piaResult != 0x35) {
      piaErrors |= RPU_RET_U11_PIA_ERROR;
   }
   piaResult = RPU_DataRead(ADDRESS_U11_B_CONTROL);
   if (piaResult != 0x34) {
      piaErrors |= RPU_RET_U11_PIA_ERROR;
   }

   return piaErrors;
}


/******************************************************
 *   Helper Functions
 */

void RPU_InitNativeAudio() {
#if (RPU_OS_HARDWARE_REV >= 2 && defined(RPU_OS_USE_SB300))
   RPU_InitSB300();
   RPU_PlaySB300StartupBeep();
#endif
}

void RPU_PlayNativeSound(uint8_t soundByte) {
#if defined(RPU_OS_USE_DASH51)
   RPU_PlaySoundDash51(soundByte);
#elif defined(RPU_OS_USE_S_AND_T)
   RPU_PlaySoundSAndT(soundByte);
#elif defined(RPU_OS_USE_SB100)
   RPU_PlaySB100(soundByte);
#else
   (void)soundByte;
#endif
}

#if defined(RPU_OS_USE_SB100) && (RPU_OS_HARDWARE_REV == 2)
void RPU_PlayNativeChime(uint8_t soundByte) {
   RPU_PlaySB100Chime(soundByte);
}
#endif


/******************************************************
 *   Interrupt Service Routines
 */

static volatile int numberOfU10Interrupts = 0;
static volatile int numberOfU11Interrupts = 0;
static volatile uint8_t InsideZeroCrossingInterrupt = 0;

// Display refresh ISR (ARCH 1 — Bally/Stern)
// See bsp_bsos.h. This is called somewhere near 319Hz to refresh the displays
ISR(TIMER1_COMPA_vect) {
   RPU::displays.serviceISR();
}

// IRQ handler: services self-test switch (U10A CA1), display interrupt (U11A CA1),
// and zero-crossing / lamp-zeroCrossingISR / solenoid-zeroCrossingISR (U10B CB1)
static void InterruptService3() {
   const uint8_t u10AControl = RPU_DataRead(ADDRESS_U10_A_CONTROL);
   if (u10AControl & 0x80) {
      if (RPU_DataRead(ADDRESS_U10_A_CONTROL) & 0x80) {
         RPU::switches.pushSelfTest();
      }
      RPU_DataRead(ADDRESS_U10_A);
   }

   // Clear any spurious U11B interrupt
   const uint8_t u11BControl = RPU_DataRead(ADDRESS_U11_B_CONTROL);
   if (u11BControl & 0x80) {
      RPU_DataRead(ADDRESS_U11_B);
   }

   const uint8_t u11AControl = RPU_DataRead(ADDRESS_U11_A_CONTROL);
   const uint8_t u10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);

   if (u11AControl & 0x80) {
      RPU_DataRead(ADDRESS_U11_A);
      numberOfU11Interrupts += 1;
   }

   if ((u10BControl & 0x80) && (InsideZeroCrossingInterrupt == 0)) {
      InsideZeroCrossingInterrupt = InsideZeroCrossingInterrupt + 1;

      const uint8_t backupU10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);
      const uint8_t backup10A = RPU_DataRead(ADDRESS_U10_A);

      RPU::switches.zeroCrossingISR();
      RPU::solenoids.zeroCrossingISR();
      RPU::lamps.zeroCrossingISR();

      interrupts();
      noInterrupts();

      InsideZeroCrossingInterrupt = 0;
      RPU_DataWrite(ADDRESS_U10_A, backup10A);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl);

      RPU_DataRead(ADDRESS_U10_B);
      numberOfU10Interrupts += 1;
   }
}

static void RPU_HookInterrupts() {
   cli();
   TCCR1A = 0;
   TCCR1B = 0;
   TCNT1 = 0;
   OCR1A = RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL;
   TCCR1B |= (1 << WGM12);
   TCCR1B |= (1 << CS12) | (1 << CS10);
   TIMSK1 |= (1 << OCIE1A);
   sei();

   attachInterrupt(digitalPinToInterrupt(2), InterruptService3, LOW);
}


/******************************************************
 *   Credit/Reset switch detection (used by hw_rev4 and hw_rev101_102 EarlyInit)
 */

bool CheckCreditResetSwitchArch1(uint8_t creditResetSwitch) {
   InitializeU10PIA();
   InitializeU11PIA();

   const uint8_t strobeNum = 0x01 << (creditResetSwitch / 8);
   const uint8_t switchNum = 0x01 << (creditResetSwitch % 8);

   RPU_DataWrite(ADDRESS_U10_A, strobeNum);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x34);

   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);

   uint8_t curSwitchByte = RPU_DataRead(ADDRESS_U10_B);

   RPU_DataWrite(ADDRESS_U10_A, 0x00);

   return (curSwitchByte & switchNum) != 0;
}


/******************************************************
 *   Main loop helper
 */

void RPU_Update(unsigned long currentTime) {
   RPU_DataRead(0);
   RPU::lamps.applyFlash(currentTime);
   RPU::solenoids.updateTimed(currentTime);
}


/******************************************************
 *   Initialization
 */

uint16_t RPU_InitializeMPU(uint16_t initOptions, uint8_t creditResetSwitch) {
   uint16_t retVal = 0;

   delay(100);

   retVal = RPU_InitializeBSP(initOptions, creditResetSwitch);

   RPU_DEBUG_MESSAGE("* About to init Arduino ports\n");
   RPU_DEBUG_DELAY(100);

   RPU_HW_SetupPorts(retVal);

   RPU_DEBUG_MESSAGE("* About to data read\n");
   RPU_DEBUG_DELAY(100);
   RPU_DataRead(0);

   RPU_DEBUG_MESSAGE("* DataRead(0) done\n");
   RPU_DEBUG_DELAY(100);

   InitializeU10PIA();
   InitializeU11PIA();

#ifdef RPU_OS_USE_DIP_SWITCHES
   RPU::dipSwitches.read();
#endif

   RPU_DataRead(0);

   RPU::solenoids.reset();
   RPU::switches.reset();
   RPU::displays.reset();
   RPU::lamps.reset();

   RPU_DEBUG_MESSAGE("* About to hook interrupts\n");
   RPU_DEBUG_DELAY(100);

   RPU_HookInterrupts();
   RPU_DataRead(0);

   RPU_DataRead(ADDRESS_U11_A);
   RPU_DataRead(ADDRESS_U11_B);
   RPU_DataRead(ADDRESS_U10_A);
   RPU_DataRead(ADDRESS_U10_B);

   if (initOptions & RPU_CMD_PERFORM_MPU_TEST) {
      retVal |= RPU_TestPIAs();
   }
   RPU_DataRead(0);

   return retVal;
}

#endif