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
#include "RPU_DipSwitches.h"
#include "RPU_Switches.h"
#include "RPU_Display.h"
#include "RPU_Lamps.h"
#include "RPU_Solenoids.h"
#include <Arduino.h>


/******************************************************
 *   Defines and library variables
 */
#if !defined(RPU_MPU_BUILD_FOR_6800) || (RPU_MPU_BUILD_FOR_6800 == 1)

#if (RPU_OS_HARDWARE_REV == 102)
static bool UsesM6800Processor = true; // Version 102 performs 6800 detection...
#else
constexpr bool UsesM6800Processor = true;
#endif

#else
constexpr bool UsesM6800Processor = false;
#endif



#if (RPU_OS_HARDWARE_REV == 1) or (RPU_OS_HARDWARE_REV == 2)

#if !defined(__AVR_ATmega328P__)
#error "Versions 1 & 2 use the Arduino Nano 328, check your compiler settings"
#endif

void RPU_DataWrite(int address, uint8_t data) {
   // Set data pins to output
   // Make pins 5-7 output (and pin 3 for R/W)
   DDRD = DDRD | 0xE8;
   // Make pins 8-12 output
   DDRB = DDRB | 0x1F;

   // Set R/W to LOW
   PORTD = (PORTD & 0xF7);

   // Put data on pins
   // Put lower three bits on 5-7
   PORTD = (PORTD & 0x1F) | ((data & 0x07) << 5);
   // Put upper five bits on 8-12
   PORTB = (PORTB & 0xE0) | (data >> 3);

   // Set up address lines
   PORTC = (PORTC & 0xE0) | address;

   // Wait for a falling edge of the clock
   while ((PIND & 0x10))
      ;

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTC = PORTC | 0x20;

   // Wait while clock is low
   while (!(PIND & 0x10))
      ;

   // Wait while clock is high
   while ((PIND & 0x10))
      ;

   // Wait while clock is low
   while (!(PIND & 0x10))
      ;

   // Set VMA OFF
   PORTC = PORTC & 0xDF;

   // Unset address lines
   PORTC = PORTC & 0xE0;

   // Set R/W back to HIGH
   PORTD = (PORTD | 0x08);

   // Set data pins to input
   // Make pins 5-7 input
   DDRD = DDRD & 0x1F;
   // Make pins 8-12 input
   DDRB = DDRB & 0xE0;
}

uint8_t RPU_DataRead(int address) {
   // Set data pins to input
   // Make pins 5-7 input
   DDRD = DDRD & 0x1F;
   // Make pins 8-12 input
   DDRB = DDRB & 0xE0;

   // Set R/W to HIGH
   DDRD = DDRD | 0x08;
   PORTD = (PORTD | 0x08);

   // Set up address lines
   PORTC = (PORTC & 0xE0) | address;

   // Wait for a falling edge of the clock
   while ((PIND & 0x10))
      ;

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTC = PORTC | 0x20;

   // Wait a full clock cycle to make sure data lines are ready
   // (important for faster clocks)
   // Wait while clock is low
   while (!(PIND & 0x10))
      ;

   // Wait for a falling edge of the clock
   while ((PIND & 0x10))
      ;

   // Wait while clock is low
   while (!(PIND & 0x10))
      ;

   uint8_t inputData = (PIND >> 5) | (PINB << 3);

   // Set VMA OFF
   PORTC = PORTC & 0xDF;

   // Wait for a falling edge of the clock
   // Doesn't seem to help  while((PIND & 0x10));

   // Set R/W to LOW
   PORTD = (PORTD & 0xF7);

   // Clear address lines
   PORTC = (PORTC & 0xE0);

   return inputData;
}

static void WaitClockCycle(int numCycles = 1) {
   for (int count = 0; count < numCycles; count++) {
      // Wait while clock is low
      while (!(PIND & 0x10))
         ;

      // Wait for a falling edge of the clock
      while ((PIND & 0x10))
         ;
   }
}

#elif (RPU_OS_HARDWARE_REV == 3)

#if !defined(__AVR_ATmega2560__)
#error "RPU_OS_HARDWARE_REV 3 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

// Rev 3 connections
// Pin D2 = IRQ
// Pin D3 = CLOCK
// Pin D4 = VMA
// Pin D5 = R/W
// Pin D6-12 = D0-D6
// Pin D13 = SWITCH
// Pin D14 = HALT
// Pin D15 = D7
// Pin D16-30 = A0-A14


void RPU_DataWrite(int address, uint8_t data) {
   // Set data pins to output
   DDRH = DDRH | 0x78;
   DDRB = DDRB | 0x70;
   DDRJ = DDRJ | 0x01;

   // Set R/W to LOW
   PORTE = (PORTE & 0xF7);

   // Put data on pins
   // Lower Nibble goes on PortH3 through H6
   PORTH = (PORTH & 0x87) | ((data & 0x0F) << 3);
   // Bits 4-6 go on PortB4 through B6
   PORTB = (PORTB & 0x8F) | ((data & 0x70));
   // Bit 7 goes on PortJ0
   PORTJ = (PORTJ & 0xFE) | (data >> 7);

   // Set up address lines
   PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
   PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) | ((address & 0x0010) >> 3) |
           ((address & 0x0020) >> 5);                                              // A2-A5
   PORTA = ((address & 0x3FC0) >> 6);                                              // A6-A13
   PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

   // Wait for a falling edge of the clock
   while ((PINE & 0x20))
      ;

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTG = PORTG | 0x20;

   // Wait while clock is low
   while (!(PINE & 0x20))
      ;

   // Wait while clock is high
   while ((PINE & 0x20))
      ;

   // Wait while clock is low
   while (!(PINE & 0x20))
      ;

   // Set VMA OFF
   PORTG = PORTG & 0xDF;

   // Unset address lines
   PORTH = (PORTH & 0xFC);
   PORTD = (PORTD & 0xF0);
   PORTA = 0;
   PORTC = (PORTC & 0x3F);

   // Set R/W back to HIGH
   PORTE = (PORTE | 0x08);

   // Set data pins to input
   DDRH = DDRH & 0x87;
   DDRB = DDRB & 0x8F;
   DDRJ = DDRJ & 0xFE;
}

uint8_t RPU_DataRead(int address) {
   // Set data pins to input
   DDRH = DDRH & 0x87;
   DDRB = DDRB & 0x8F;
   DDRJ = DDRJ & 0xFE;

   // Set R/W to HIGH
   DDRE = DDRE | 0x08;
   PORTE = (PORTE | 0x08);

   // Set up address lines
   PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
   PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) | ((address & 0x0010) >> 3) |
           ((address & 0x0020) >> 5);                                              // A2-A5
   PORTA = ((address & 0x3FC0) >> 6);                                              // A6-A13
   PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

   // Wait for a falling edge of the clock
   while ((PINE & 0x20))
      ;

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTG = PORTG | 0x20;

   // Wait a full clock cycle to make sure data lines are ready
   // (important for faster clocks)
   // Wait while clock is low
   while (!(PINE & 0x20))
      ;

   // Wait for a falling edge of the clock
   while ((PINE & 0x20))
      ;

   // Wait while clock is low
   while (!(PINE & 0x20))
      ;

   uint8_t inputData;
   inputData = (PINH & 0x78) >> 3;
   inputData |= (PINB & 0x70);
   inputData |= PINJ << 7;

   // Set VMA OFF
   PORTG = PORTG & 0xDF;

   // Set R/W to LOW
   PORTE = (PORTE & 0xF7);

   // Unset address lines
   PORTH = (PORTH & 0xFC);
   PORTD = (PORTD & 0xF0);
   PORTA = 0;
   PORTC = (PORTC & 0x3F);

   return inputData;
}

// static void WaitClockCycle(int numCycles = 1) {
//    for (int count = 0; count < numCycles; count++) {
//       // Wait while clock is low
//       while (!(PINE & 0x20))
//          ;
//
//       // Wait for a falling edge of the clock
//       while ((PINE & 0x20))
//          ;
//    }
// }

#elif (RPU_OS_HARDWARE_REV == 4)

#if !defined(__AVR_ATmega2560__)
#error "RPU_OS_HARDWARE_REV 4 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

// Rev 3 connections
// Pin D2 = IRQ
// Pin D3 = CLOCK
// Pin D4 = VMA
// Pin D5 = R/W
// Pin D6-12 = D0-D6
// Pin D13 = SWITCH
// Pin D14 = HALT
// Pin D15 = D7
// Pin D16-30 = A0-A14

constexpr uint8_t RPU_VMA_PIN = 40;
constexpr uint8_t RPU_RW_PIN = 3;
constexpr uint8_t RPU_PHI2_PIN = 39;
constexpr uint8_t RPU_SWITCH_PIN = 38;
constexpr uint8_t RPU_BUFFER_DISABLE = 5;
constexpr uint8_t RPU_HALT_PIN = 41;
constexpr uint8_t RPU_RESET_PIN = 42;
constexpr uint8_t RPU_DIAGNOSTIC_PIN = 44;

constexpr bool RPU_PINS_OUTPUT = true;
constexpr bool RPU_PINS_INPUT = false;

static void RPU_SetAddressPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 16; count++) {
      pinMode(A0 + count, pinsOutput ? OUTPUT : INPUT);
   }
}

static void RPU_SetDataPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 8; count++) {
      pinMode(22 + count, pinsOutput ? OUTPUT : INPUT);
   }
}

// REVISION 4 HARDWARE
void RPU_DataWrite(int address, uint8_t data) {
   // Set data pins to output
   DDRA = 0xFF;

   // Set R/W to LOW
   PORTE = (PORTE & 0xDF);

   // Put data on pins
   PORTA = data;

   // Set up address lines
   PORTF = (uint8_t)(address & 0x00FF);
   PORTK = (uint8_t)(address / 256);

   if (UsesM6800Processor) {
      // Wait for a falling edge of the clock
      while ((PING & 0x04))
         ;
   } else {
      // Set clock low (PG2) (if 6802/8)
      PORTG &= ~0x04;
   }

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTG = PORTG | 0x02;

   if (UsesM6800Processor) {
      // Wait while clock is low
      while (!(PING & 0x04))
         ;

      // Wait while clock is high
      while ((PING & 0x04))
         ;

      // Wait while clock is low
      while (!(PING & 0x04))
         ;
   } else {
      // Set clock high
      PORTG |= 0x04;

      // Set clock low
      PORTG &= ~0x04;

      // Set clock high
      PORTG |= 0x04;
   }

   // Set VMA OFF
   PORTG = PORTG & 0xFD;

   // Unset address lines
   PORTF = 0x00;
   PORTK = 0x00;

   // Set R/W back to HIGH
   PORTE = (PORTE | 0x20);

   // Set data pins to input
   DDRA = 0x00;
}

uint8_t RPU_DataRead(int address) {
   // Set data pins to input
   DDRA = 0x00;

   // Set R/W to HIGH
   DDRE = DDRE | 0x20;
   PORTE = (PORTE | 0x20);

   // Set up address lines
   PORTF = (uint8_t)(address & 0x00FF);
   PORTK = (uint8_t)(address / 256);

   if (UsesM6800Processor) {
      // Wait for a falling edge of the clock
      while ((PING & 0x04))
         ;
   } else {
      // Set clock low
      PORTG &= ~0x04;
   }

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTG = PORTG | 0x02;

   if (UsesM6800Processor) {
      // Wait a full clock cycle to make sure data lines are ready
      // (important for faster clocks)
      // Wait while clock is low
      while (!(PING & 0x04))
         ;

      // Wait for a falling edge of the clock
      while ((PING & 0x04))
         ;

      // Wait while clock is low
      while (!(PING & 0x04))
         ;
   } else {
      // Set clock high
      PORTG |= 0x04;

      // Set clock low
      PORTG &= ~0x04;

      // Set clock high
      PORTG |= 0x04;
   }

   uint8_t inputData;
   inputData = PINA;

   // Set VMA OFF
   PORTG = PORTG & 0xFD;

   // Set R/W to LOW
   PORTE = (PORTE & 0xDF);

   // Unset address lines
   PORTF = 0x00;
   PORTK = 0x00;

   return inputData;
}

#elif (RPU_OS_HARDWARE_REV == 100)

#if !defined(__AVR_ATmega2560__)
#error "RPU_OS_HARDWARE_REV 100 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

constexpr uint8_t RPU_VMA_PIN = 4;
constexpr uint8_t RPU_RW_PIN = 5;
constexpr uint8_t RPU_PHI2_PIN = 3;
constexpr uint8_t RPU_SWITCH_PIN = 13;
constexpr uint8_t RPU_BUFFER_DISABLE = 2;
constexpr uint8_t RPU_HALT_PIN = 14;
constexpr uint8_t RPU_RESET_PIN = 14;
constexpr bool RPU_PINS_OUTPUT = true;
constexpr bool RPU_PINS_INPUT = false;

void RPU_SetAddressPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 16; count++) {
      pinMode(16 + count, pinsOutput ? OUTPUT : INPUT);
   }
}

void RPU_SetDataPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 7; count++) {
      pinMode(6 + count, pinsOutput ? OUTPUT : INPUT);
   }
   pinMode(15, pinsOutput ? OUTPUT : INPUT);
}

// REV 100 HARDWARE
void RPU_DataWrite(int address, uint8_t data) {
   // Set data pins to output
   DDRH = DDRH | 0x78;
   DDRB = DDRB | 0x70;
   DDRJ = DDRJ | 0x01;

   // Set R/W to LOW
   PORTE = (PORTE & 0xF7);

   // Put data on pins
   // Lower Nibble goes on PortH3 through H6
   PORTH = (PORTH & 0x87) | ((data & 0x0F) << 3);
   // Bits 4-6 go on PortB4 through B6
   PORTB = (PORTB & 0x8F) | ((data & 0x70));
   // Bit 7 goes on PortJ0
   PORTJ = (PORTJ & 0xFE) | (data >> 7);

   // Set up address lines
   PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
   PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) | ((address & 0x0010) >> 3) |
           ((address & 0x0020) >> 5);                                              // A2-A5
   PORTA = ((address & 0x3FC0) >> 6);                                              // A6-A13
   PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

   // Set clock low
   PORTE &= ~0x20;

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTG = PORTG | 0x20;

   // Set clock high
   PORTE |= 0x20;

   // Set clock low
   PORTE &= ~0x20;

   // Set clock high
   PORTE |= 0x20;

   // Set VMA OFF
   PORTG = PORTG & 0xDF;

   // Unset address lines
   PORTH = (PORTH & 0xFC);
   PORTD = (PORTD & 0xF0);
   PORTA = 0;
   PORTC = (PORTC & 0x3F);

   // Set R/W back to HIGH
   PORTE = (PORTE | 0x08);

   // Set data pins to input
   DDRH = DDRH & 0x87;
   DDRB = DDRB & 0x8F;
   DDRJ = DDRJ & 0xFE;
}

uint8_t RPU_DataRead(int address) {
   // Set data pins to input
   DDRH = DDRH & 0x87;
   DDRB = DDRB & 0x8F;
   DDRJ = DDRJ & 0xFE;

   // Set R/W to HIGH
   DDRE = DDRE | 0x08;
   PORTE = (PORTE | 0x08);

   // Set up address lines
   PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
   PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) | ((address & 0x0010) >> 3) |
           ((address & 0x0020) >> 5);                                              // A2-A5
   PORTA = ((address & 0x3FC0) >> 6);                                              // A6-A13
   PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

   // Set clock low
   PORTE &= ~0x20;

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTG = PORTG | 0x20;

   // Set clock high
   PORTE |= 0x20;

   // Set clock low
   PORTE &= ~0x20;

   // Set clock high
   PORTE |= 0x20;

   uint8_t inputData;
   inputData = (PINH & 0x78) >> 3;
   inputData |= (PINB & 0x70);
   inputData |= PINJ << 7;

   // Set VMA OFF
   PORTG = PORTG & 0xDF;

   // Set R/W to LOW
   PORTE = (PORTE & 0xF7);

   // Unset address lines
   PORTH = (PORTH & 0xFC);
   PORTD = (PORTD & 0xF0);
   PORTA = 0;
   PORTC = (PORTC & 0x3F);

   return inputData;
}

// static void WaitClockCycle(int numCycles = 1) {
//    for (int count = 0; count < numCycles; count++) {
//       // Wait while clock is low
//       while (!(PINE & 0x20))
//          ;
//
//       // Wait for a falling edge of the clock
//       while ((PINE & 0x20))
//          ;
//    }
// }

#elif (RPU_OS_HARDWARE_REV == 101) || (RPU_OS_HARDWARE_REV == 102)

#if !defined(__AVR_ATmega2560__)
#error "RPU_OS_HARDWARE_REV >100 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

constexpr uint8_t RPU_VMA_PIN = 40;
constexpr uint8_t RPU_RW_PIN = 3;
constexpr uint8_t RPU_PHI2_PIN = 39;
constexpr uint8_t RPU_SWITCH_PIN = 38;
constexpr uint8_t RPU_BUFFER_DISABLE = 5;
constexpr uint8_t RPU_HALT_PIN = 41;
constexpr uint8_t RPU_RESET_PIN = 42;
constexpr uint8_t RPU_BA_PIN = 43;
constexpr uint8_t RPU_DIAGNOSTIC_PIN = 44;
constexpr uint8_t RPU_DISABLE_PHI_FROM_MPU = 7;
constexpr uint8_t RPU_DISABLE_PHI_FROM_CPU = 6;
constexpr uint8_t RPU_BOARD_SEL_0 = 30;
constexpr uint8_t RPU_BOARD_SEL_1 = 31;
constexpr uint8_t RPU_BOARD_SEL_2 = 32;
constexpr uint8_t RPU_BOARD_SEL_3 = 33;
constexpr bool RPU_PINS_OUTPUT = true;
constexpr bool RPU_PINS_INPUT = false;

static void RPU_SetAddressPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 16; count++) {
      pinMode(A0 + count, pinsOutput ? OUTPUT : INPUT);
   }
}

static void RPU_SetDataPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 8; count++) {
      pinMode(22 + count, pinsOutput ? OUTPUT : INPUT);
   }
}

// REVISION 101/102 HARDWARE
void RPU_DataWrite(int address, uint8_t data) {
   // Set data pins to output
   DDRA = 0xFF;

   // Set R/W to LOW
   PORTE = (PORTE & 0xDF);

   // Put data on pins
   PORTA = data;

   // Set up address lines
   PORTF = (uint8_t)(address & 0x00FF);
   PORTK = (uint8_t)(address / 256);

   if (UsesM6800Processor) {
      // Wait for a falling edge of the clock
      while ((PING & 0x04))
         ;
   } else {
      // Set clock low (PG2) (if 6802/8)
      PORTG &= ~0x04;
   }

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTG = PORTG | 0x02;

   if (UsesM6800Processor) {
      // Wait while clock is low
      while (!(PING & 0x04))
         ;

      // Wait while clock is high
      while ((PING & 0x04))
         ;

      // Wait while clock is low
      while (!(PING & 0x04))
         ;
   } else {
      // Set clock high
      PORTG |= 0x04;

      // Set clock low
      PORTG &= ~0x04;

      // Set clock high
      PORTG |= 0x04;
   }

   // Set VMA OFF
   PORTG = PORTG & 0xFD;

   // Unset address lines
   PORTF = 0x00;
   PORTK = 0x00;

   // Set R/W back to HIGH
   PORTE = (PORTE | 0x20);

   // Set data pins to input
   DDRA = 0x00;
}

uint8_t RPU_DataRead(int address) {
   // Set data pins to input
   DDRA = 0x00;

   // Set R/W to HIGH
   DDRE = DDRE | 0x20;
   PORTE = (PORTE | 0x20);

   // Set up address lines
   PORTF = (uint8_t)(address & 0x00FF);
   PORTK = (uint8_t)(address / 256);

   if (UsesM6800Processor) {
      // Wait for a falling edge of the clock
      while ((PING & 0x04))
         ;
   } else {
      // Set clock low
      PORTG &= ~0x04;
   }

   // Pulse VMA over one clock cycle
   // Set VMA ON
   PORTG = PORTG | 0x02;

   if (UsesM6800Processor) {
      // Wait a full clock cycle to make sure data lines are ready
      // (important for faster clocks)
      // Wait while clock is low
      while (!(PING & 0x04))
         ;

      // Wait for a falling edge of the clock
      while ((PING & 0x04))
         ;

      // Wait while clock is low
      while (!(PING & 0x04))
         ;
   } else {
      // Set clock high
      PORTG |= 0x04;

      // Set clock low
      PORTG &= ~0x04;

      // Set clock high
      PORTG |= 0x04;
   }

   const uint8_t inputData = PINA;

   // Set VMA OFF
   PORTG = PORTG & 0xFD;

   // Set R/W to LOW
   PORTE = (PORTE & 0xDF);

   // Unset address lines
   PORTF = 0x00;
   PORTK = 0x00;

   return inputData;
}

#else
#error "RPU Hardware Definition Not Recognized"
#endif

// static void TestLightOn() {
//    RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
// }

// static void TestLightOff() {
//    RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
// }

static void InitializeU10PIA() {
   // CA1 - Self Test Switch
   // CB1 - zero crossing detector
   // CA2 - NOR'd with display latch strobe
   // CB2 - lamp strobe 1
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
   // CA2 - lamp strobe 2
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
   solenoids.initDefault();
}

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
 *   Initialization and ISR Functions
 */
#if (RPU_OS_HARDWARE_REV == 102)
bool CheckForMPUClock() {
   pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
   digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 1);
   pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
   digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);
   pinMode(RPU_PHI2_PIN, INPUT_PULLUP);

   unsigned long startTime = millis();
   int sawClockLow = 0;
   int sawClockHigh = 0;
   while (millis() < (startTime + 10)) {
      if (PING & 0x04) {
         sawClockHigh += 1;
      } else {
         sawClockLow += 1;
      }
   }

   if (sawClockLow > 25 && sawClockHigh > 25) {
      return true;
   }

   // At this point, since we didn't see the mpu clock, we
   // can assume that the clock is generated by the CPU,
   // but we can check if we want
   digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 0);

   sawClockLow = 0;
   sawClockHigh = 0;
   startTime = millis();
   uint8_t lastState = 0;
   for (int count = 0; count < 1000; count++) {
      if (PING & 0x04 && !lastState) {
         sawClockHigh += 1;
         lastState = 1;
      } else if (lastState) {
         sawClockLow += 1;
         lastState = 0;
      }
   }

   digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);

   return false;
}
#endif

static volatile int numberOfU10Interrupts = 0;
static volatile int numberOfU11Interrupts = 0;
static volatile uint8_t InsideZeroCrossingInterrupt = 0;

// INTERRUPT SERVICE ROUTINE
// for ARCH 1 (B/S)
ISR(TIMER1_COMPA_vect) { // This is the interrupt request
   displays.serviceISR();
}

/**
 * This interrupt services the digital pin 2 (zero crossing)
 */
static void InterruptService3() {
   const uint8_t u10AControl = RPU_DataRead(ADDRESS_U10_A_CONTROL);
   if (u10AControl & 0x80) {
      // self test switch
      if (RPU_DataRead(ADDRESS_U10_A_CONTROL) & 0x80) {
         switches.pushSelfTest();
      }
      RPU_DataRead(ADDRESS_U10_A);
   }

   // If we get a weird interrupt from U11B, clear it
   const uint8_t u11BControl = RPU_DataRead(ADDRESS_U11_B_CONTROL);
   if (u11BControl & 0x80) {
      RPU_DataRead(ADDRESS_U11_B);
   }

   const uint8_t u11AControl = RPU_DataRead(ADDRESS_U11_A_CONTROL);
   const uint8_t u10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);

   // If the interrupt bit on the display interrupt is on, do the display refresh
   if (u11AControl & 0x80) {
      RPU_DataRead(ADDRESS_U11_A);
      numberOfU11Interrupts += 1;
   }

   // If the IRQ bit of U10BControl is set, do the Zero-crossing interrupt handler
   if ((u10BControl & 0x80) && (InsideZeroCrossingInterrupt == 0)) {
      InsideZeroCrossingInterrupt = InsideZeroCrossingInterrupt + 1;

      // Backup contents of U10A and U10B_CONTROL
      const uint8_t backupU10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);
      const uint8_t backup10A = RPU_DataRead(ADDRESS_U10_A);

      switches.service();

      solenoids.service();

      lamps.strobe();

      interrupts();
      noInterrupts();

      InsideZeroCrossingInterrupt = 0;
      RPU_DataWrite(ADDRESS_U10_A, backup10A);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl);

      // Read U10B to clear interrupt
      RPU_DataRead(ADDRESS_U10_B);
      numberOfU10Interrupts += 1;
   }
}

static void RPU_HookInterrupts() {
   // Hook up the interrupt

   cli();
   // set timer1 interrupt at 1Hz
   TCCR1A = 0; // set entire TCCR1A register to 0
   TCCR1B = 0; // same for TCCR1B
   TCNT1 = 0;  // initialize counter value to 0
   // set compare match register for selected increment
   OCR1A = RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL;
   // turn on CTC mode
   TCCR1B |= (1 << WGM12);
   // Set CS10 and CS12 bits for 1024 prescaler
   TCCR1B |= (1 << CS12) | (1 << CS10);
   // enable timer compare interrupt
   TIMSK1 |= (1 << OCIE1A);
   sei();

   attachInterrupt(digitalPinToInterrupt(2), InterruptService3, LOW);
}

#if (RPU_OS_HARDWARE_REV == 1) or (RPU_OS_HARDWARE_REV == 2)
static bool LookFor6800Activity() {
   // Assume Arduino pins all start as input
   unsigned long startTime = millis();
   bool sawHigh = false;
   bool sawLow = false;
   // for one second, look for activity on the VMA line (A5)
   // If we see anything, then the MPU is active so we shouldn't run
   while ((millis() - startTime) < 1000) {
      if (PINC & 0x20) {
         sawHigh = true;
      } else {
         sawLow = true;
      }
   }
   // If we saw both a high and low signal, then someone is toggling the
   // VMA line, so we should hang here forever (until reset)
   if (sawHigh && sawLow) {
      return true;
   }
   return false;
}
#endif

static void SetupArduinoPorts() {
#if (RPU_OS_HARDWARE_REV == 1)
   // Arduino A0 = MPU A0
   // Arduino A1 = MPU A1
   // Arduino A2 = MPU A3
   // Arduino A3 = MPU A4
   // Arduino A4 = MPU A7
   // Arduino A5 = MPU VMA
   // Set up the address lines A0-A7 as output
   DDRC = DDRC | 0x3F;
   // Set up D13 as address line A5 (and set it low)
   DDRB = DDRB | 0x20;
   PORTB = PORTB & 0xDF;
   // Set up control lines & data lines
   DDRD = DDRD & 0xEB;
   DDRD = DDRD | 0xE8;
   // Set VMA OFF
   PORTC = PORTC & 0xDF;
   // Set R/W to HIGH
   PORTD = (PORTD | 0x08);
#elif (RPU_OS_HARDWARE_REV == 2)
   // Set up the address lines A0-A7 as output
   DDRC = DDRC | 0x3F;
   // Set up D13 as address line A7 (and set it high)
   DDRB = DDRB | 0x20;
   PORTB = PORTB | 0x20;
   // Set up control lines & data lines
   DDRD = DDRD & 0xEB;
   DDRD = DDRD | 0xE8;
   // Set VMA OFF
   PORTC = PORTC & 0xDF;
   // Set R/W to HIGH
   PORTD = (PORTD | 0x08);
#elif (RPU_OS_HARDWARE_REV == 3)
   pinMode(3, INPUT);  // CLK
   pinMode(4, OUTPUT); // VMA
   pinMode(5, OUTPUT); // R/W
   for (uint8_t count = 6; count < 13; count++) {
      pinMode(count, INPUT); // D0-D6
   }
   pinMode(13, INPUT);  // Switch
   pinMode(14, OUTPUT); // Halt
   pinMode(15, INPUT);  // D7
   for (uint8_t count = 16; count < 32; count++) {
      pinMode(count, OUTPUT); // Address lines are output
   }
   digitalWrite(5, HIGH); // Set R/W line high (Read)
   digitalWrite(4, LOW);  // Set VMA line LOW
#elif (RPU_OS_HARDWARE_REV == 4)
#endif
}

bool CheckCreditResetSwitchArch1(uint8_t creditResetSwitch) {
   // Check for credit button
   InitializeU10PIA();
   InitializeU11PIA();

   const uint8_t strobeNum = 0x01 << (creditResetSwitch / 8);
   const uint8_t switchNum = 0x01 << (creditResetSwitch % 8);

   RPU_DataWrite(ADDRESS_U10_A, strobeNum);
   // Turn off U10:CB2 if it's on (because it strobes the last bank of dip switches
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x34);

   // Delay for switch capacitors to charge
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);

   // Read the switches
   uint8_t curSwitchByte = RPU_DataRead(ADDRESS_U10_B);

   // Unset the strobe
   RPU_DataWrite(ADDRESS_U10_A, 0x00);

   if (curSwitchByte & switchNum) {
      return true;
   }
   return false;
}


void RPU_Update(unsigned long currentTime) {
   RPU_DataRead(0);

   lamps.applyFlash(currentTime);
   solenoids.updateTimed(currentTime);
}

// This function should eventually support auto-detect and initialize the appropriate
// ISRs for the detected architecture.
uint16_t RPU_InitializeMPU(uint16_t initOptions, uint8_t creditResetSwitch) {
   uint16_t retVal = 0;

   // Wait for board to boot
   delayMicroseconds(50000);
   delayMicroseconds(50000);

#if (RPU_OS_HARDWARE_REV == 1) or (RPU_OS_HARDWARE_REV == 2)
   (void)creditResetSwitch;

   if (initOptions & (RPU_CMD_BOOT_ORIGINAL | RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET |
                      RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED | RPU_CMD_AUTODETECT_ARCHITECTURE)) {
      retVal |= RPU_RET_OPTION_NOT_SUPPORTED;
   }

   if (LookFor6800Activity()) {
      if (initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) {
         retVal |= RPU_RET_ORIGINAL_CODE_REQUESTED;
         return retVal;
      } else {
         while (1)
            ;
      }
   }
#elif (RPU_OS_HARDWARE_REV == 3)
   (void)creditResetSwitch;

   RPU_DEBUG_MESSAGE("* Starting Setup for Rev 3\n");

   if (initOptions &
       (RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET | RPU_CMD_AUTODETECT_ARCHITECTURE)) {
      retVal |= RPU_RET_OPTION_NOT_SUPPORTED;
   }

   pinMode(13, INPUT);
   bool switchStateClosed = digitalRead(13) ? false : true;
   bool bootToOriginal = false;

   if (switchStateClosed) {
      retVal |= RPU_RET_SELECTOR_SWITCH_ON;
   }

   if ((initOptions & RPU_CMD_BOOT_ORIGINAL) || (switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED)) ||
       (!switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED))) {
      bootToOriginal = true;
   }

   if (bootToOriginal) {
      RPU_DEBUG_MESSAGE("* Asked to boot to original\n");
      RPU_DEBUG_DELAY(100);

      // Let the 680X run
      pinMode(14, OUTPUT); // Halt
      digitalWrite(14, HIGH);
      if (initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) {
         retVal |= RPU_RET_ORIGINAL_CODE_REQUESTED;
         return retVal;
      } else {
         while (1)
            ;
      }
   } else {
      // Switch indicates the Arduino should run, so HALT the 680X
      pinMode(14, OUTPUT); // Halt
      digitalWrite(14, LOW);
   }

#elif (RPU_OS_HARDWARE_REV == 4) || (RPU_OS_HARDWARE_REV >= 101)
   // put the 680X buffers into tri-state
   pinMode(RPU_BUFFER_DISABLE, OUTPUT);
   digitalWrite(RPU_BUFFER_DISABLE, 1);

   // Set /HALT low so the processor doesn't come online
   // (on some hardware, HALT & RESET are combined)
   pinMode(RPU_HALT_PIN, OUTPUT);
   digitalWrite(RPU_HALT_PIN, 0);
   pinMode(RPU_RESET_PIN, OUTPUT);
   digitalWrite(RPU_RESET_PIN, 0);

   // Set VMA, R/W to OUTPUT
   pinMode(RPU_VMA_PIN, OUTPUT);
   pinMode(RPU_RW_PIN, OUTPUT);
   RPU_SetAddressPinsDirection(RPU_PINS_OUTPUT);

#if (RPU_OS_HARDWARE_REV == 102)
   if (CheckForMPUClock()) {
      UsesM6800Processor = true;
   } else {
      UsesM6800Processor = false;
   }
#endif

   // Set PHI2 depending on processor type
   if (UsesM6800Processor) {
      pinMode(RPU_PHI2_PIN, INPUT);
   } else {
      pinMode(RPU_PHI2_PIN, OUTPUT);
   }

   delay(1000);
   //  RPU_DataWrite(ADDRESS_SB100, 0x01);
   bool switchStateClosed = false;
   pinMode(RPU_SWITCH_PIN, INPUT);
   if (digitalRead(RPU_SWITCH_PIN)) {
      switchStateClosed = true;
      retVal |= RPU_RET_SELECTOR_SWITCH_ON;
   }

   bool creditResetButtonHit = false;
   if (creditResetSwitch != 0xFF && (initOptions & (RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET))) {
      // We have to check the credit/reset button to honor the init request
      creditResetButtonHit = CheckCreditResetSwitchArch1(creditResetSwitch);
      if (creditResetButtonHit) {
         retVal |= RPU_RET_CREDIT_RESET_BUTTON_HIT;
      }
   }

   bool bootToOriginal = false;
   if ((initOptions & RPU_CMD_BOOT_ORIGINAL) || (switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED)) ||
       (!switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED)) ||
       (creditResetButtonHit && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET)) ||
       (!creditResetButtonHit && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET))) {
      bootToOriginal = true;
   }

   if (bootToOriginal) {
      // If the options guide us to original code, boot to original
      pinMode(RPU_BUFFER_DISABLE, OUTPUT); // IRQ
      // Turn on the tri-state buffers
      digitalWrite(RPU_BUFFER_DISABLE, 0);

      pinMode(RPU_PHI2_PIN, INPUT); // CLOCK
      pinMode(RPU_VMA_PIN, INPUT);  // VMA
      pinMode(RPU_RW_PIN, INPUT);   // R/W

#if (RPU_OS_HARDWARE_REV == 102)
      // We need to make sure the clock direction
      // buffers are set the correct direction
      if (UsesM6800Processor) {
         pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
         digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 0);
         pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
         digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);
         retVal |= RPU_RET_6800_DETECTED;
      } else {
         pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
         digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 1);
         pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
         digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 0);
         retVal |= RPU_RET_6802_OR_8_DETECTED;
      }
#endif

      // Set all the pins to input so they'll stay out of the way
      RPU_SetDataPinsDirection(RPU_PINS_INPUT);
      RPU_SetAddressPinsDirection(RPU_PINS_INPUT);

      // Set /HALT high
      pinMode(RPU_HALT_PIN, OUTPUT);
      digitalWrite(RPU_HALT_PIN, 1);
      pinMode(RPU_RESET_PIN, OUTPUT);
      digitalWrite(RPU_RESET_PIN, 1);

      retVal |= RPU_RET_ORIGINAL_CODE_REQUESTED;
      if (!(initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN)) {
         while (1)
            ;
      } else {
         return retVal;
      }
   }

#endif

   RPU_DEBUG_MESSAGE("* About to init Arduino ports\n");
   RPU_DEBUG_DELAY(100);

   SetupArduinoPorts();

   // Prep the address bus (all lines zero)
   RPU_DEBUG_MESSAGE("* About to data read\n");
   RPU_DEBUG_DELAY(100);
   RPU_DataRead(0);

   RPU_DEBUG_MESSAGE("* DataRead(0) done\n");
   RPU_DEBUG_DELAY(100);

   // Set up the PIAs
   InitializeU10PIA();
   InitializeU11PIA();

   // Read values from MPU dip switches
#ifdef RPU_OS_USE_DIP_SWITCHES
   dipSwitches.read();
#endif

#if (RPU_OS_HARDWARE_REV == 4) || (RPU_OS_HARDWARE_REV > 100)
   pinMode(RPU_DIAGNOSTIC_PIN, INPUT);
   if (digitalRead(RPU_DIAGNOSTIC_PIN) == 1) {
      retVal |= RPU_RET_DIAGNOSTIC_REQUESTED;
   }
#endif

   // Reset address bus
   RPU_DataRead(0);

   solenoids.reset();
   switches.reset();
   displays.reset();
   lamps.reset();

   RPU_DEBUG_MESSAGE("* About to hook interrupts\n");
   RPU_DEBUG_DELAY(100);

   RPU_HookInterrupts();
   RPU_DataRead(0); // Reset address bus

   // Clear all possible interrupts by reading the registers
   RPU_DataRead(ADDRESS_U11_A);
   RPU_DataRead(ADDRESS_U11_B);
   RPU_DataRead(ADDRESS_U10_A);
   RPU_DataRead(ADDRESS_U10_B);
   if (initOptions & RPU_CMD_PERFORM_MPU_TEST) {
      retVal |= RPU_TestPIAs();
   }
   RPU_DataRead(0); // Reset address bus

   return retVal;
}
