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
#include "CircularQueue.h"
#include "RPU_Addresses.h"
#include "RPU_config.h"
#include "RPU_Debug.h"
#include "TimedStack.h"
#include <Arduino.h>
#include <EEPROM.h>

static int NumGameSwitches = 0;
static int NumGamePrioritySwitches = 0;
static const PlayfieldAndCabinetSwitch* GameSwitches = NULL;

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

constexpr unsigned RPU_NUM_SOLENOIDS = 15;
constexpr unsigned NUM_SWITCH_BYTES = 5;
constexpr unsigned NUM_SWITCH_BYTES_ON_U10_PORT_A = 5;
constexpr unsigned MAX_NUM_SWITCHES = 40;
constexpr unsigned DEFAULT_SOLENOID_STATE = 0x9F;


// Global variables
static volatile uint8_t DisplayDigits[5][RPU_OS_NUM_DIGITS];
static volatile uint8_t DisplayDigitEnable[5];
static volatile bool DisplayOffCycle = false;
static volatile uint8_t CurrentDisplayDigit = 0;
static volatile uint8_t LampStates[RPU_NUM_LAMP_BANKS], LampDim1[RPU_NUM_LAMP_BANKS], LampDim2[RPU_NUM_LAMP_BANKS];
static volatile uint8_t LampFlashPeriod[RPU_MAX_LAMPS];
static uint8_t DimDivisor1 = 2;
static uint8_t DimDivisor2 = 3;

volatile uint8_t SwitchesMinus2[NUM_SWITCH_BYTES];
volatile uint8_t SwitchesMinus1[NUM_SWITCH_BYTES];
volatile uint8_t SwitchesNow[NUM_SWITCH_BYTES];

#ifdef RPU_OS_USE_DIP_SWITCHES
static uint8_t DipSwitches[4];
#endif

#if (RPU_OS_HARDWARE_REV > 2)
#define SOLENOID_STACK_SIZE 150
#else
#define SOLENOID_STACK_SIZE 60
#endif
#define SOLENOID_STACK_EMPTY 0xFF
static CircularQueue<uint8_t, SOLENOID_STACK_SIZE, SOLENOID_STACK_EMPTY> SolenoidStack;
static bool SolenoidStackEnabled = true;
static volatile uint8_t CurrentSolenoidByte = 0xFF;
static volatile uint8_t RevertSolenoidBit = 0x00;
static volatile uint8_t NumCyclesBeforeRevertingSolenoidByte = 0;

#define TIMED_SOLENOID_STACK_SIZE 30
static TimedStack<TimedSolenoidEntry, TIMED_SOLENOID_STACK_SIZE> TimedSolenoidStack;

class SwitchStackClass : public CircularQueue<uint8_t, 60, 0xff> {
public:
   bool push(uint8_t data) {

      // Self test is a special case - there's no good way to debounce it
      // so if it's already first on the stack, ignore it
      if (data == SW_SELF_TEST_SWITCH) {
         if (!isEmpty() && peek() == SW_SELF_TEST_SWITCH) {
            return false;
         }
      }

      return CircularQueue<uint8_t, 60, 0xff>::push(data);
   }
};

static SwitchStackClass SwitchStack;


#if (RPU_OS_HARDWARE_REV == 1) or (RPU_OS_HARDWARE_REV == 2)

#if defined(__AVR_ATmega2560__)
#error "ATMega requires RPU_OS_HARDWARE_REV of 3, check RPU_Config.h and adjust settings"
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

void WaitClockCycle(int numCycles = 1) {
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

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV 3 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

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

void WaitClockCycle(int numCycles = 1) {
   for (int count = 0; count < numCycles; count++) {
      // Wait while clock is low
      while (!(PINE & 0x20))
         ;

      // Wait for a falling edge of the clock
      while ((PINE & 0x20))
         ;
   }
}

#elif (RPU_OS_HARDWARE_REV == 4)

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

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV 4 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#define RPU_VMA_PIN 40
#define RPU_RW_PIN 3
#define RPU_PHI2_PIN 39
#define RPU_SWITCH_PIN 38
#define RPU_BUFFER_DISABLE 5
#define RPU_HALT_PIN 41
#define RPU_RESET_PIN 42
#define RPU_DIAGNOSTIC_PIN 44
#define RPU_PINS_OUTPUT true
#define RPU_PINS_INPUT false

void RPU_SetAddressPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 16; count++) {
      pinMode(A0 + count, pinsOutput ? OUTPUT : INPUT);
   }
}

void RPU_SetDataPinsDirection(bool pinsOutput) {
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

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV 100 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#define RPU_VMA_PIN 4
#define RPU_RW_PIN 5
#define RPU_PHI2_PIN 3
#define RPU_SWITCH_PIN 13
#define RPU_BUFFER_DISABLE 2
#define RPU_HALT_PIN 14
#define RPU_RESET_PIN 14
#define RPU_PINS_OUTPUT true
#define RPU_PINS_INPUT false

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

void WaitClockCycle(int numCycles = 1) {
   for (int count = 0; count < numCycles; count++) {
      // Wait while clock is low
      while (!(PINE & 0x20))
         ;

      // Wait for a falling edge of the clock
      while ((PINE & 0x20))
         ;
   }
}

#elif (RPU_OS_HARDWARE_REV == 101) || (RPU_OS_HARDWARE_REV == 102)

#if defined(__AVR_ATmega328P__)
#error "RPU_OS_HARDWARE_REV >100 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#define RPU_VMA_PIN 40
#define RPU_RW_PIN 3
#define RPU_PHI2_PIN 39
#define RPU_SWITCH_PIN 38
#define RPU_BUFFER_DISABLE 5
#define RPU_HALT_PIN 41
#define RPU_RESET_PIN 42
#define RPU_BA_PIN 43
#define RPU_DIAGNOSTIC_PIN 44
#define RPU_DISABLE_PHI_FROM_MPU 7
#define RPU_DISABLE_PHI_FROM_CPU 6
#define RPU_BOARD_SEL_0 30
#define RPU_BOARD_SEL_1 31
#define RPU_BOARD_SEL_2 32
#define RPU_BOARD_SEL_3 33
#define RPU_PINS_OUTPUT true
#define RPU_PINS_INPUT false

void RPU_SetAddressPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 16; count++) {
      pinMode(A0 + count, pinsOutput ? OUTPUT : INPUT);
   }
}

void RPU_SetDataPinsDirection(bool pinsOutput) {
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

#else
#error "RPU Hardware Definition Not Recognized"
#endif

void TestLightOn() {
   RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
}

void TestLightOff() {
   RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
}

void InitializeU10PIA() {
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

#ifdef RPU_OS_USE_DIP_SWITCHES
void ReadDipSwitches() {
   uint8_t backupU10A = RPU_DataRead(ADDRESS_U10_A);
   uint8_t backupU10BControl = RPU_DataRead(ADDRESS_U10_B_CONTROL);

   // Turn on Switch strobe 5 & Read Switches
   RPU_DataWrite(ADDRESS_U10_A, 0x20);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
   // Wait for switch capacitors to charge
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   DipSwitches[0] = RPU_DataRead(ADDRESS_U10_B);

   // Turn on Switch strobe 6 & Read Switches
   RPU_DataWrite(ADDRESS_U10_A, 0x40);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
   // Wait for switch capacitors to charge
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   DipSwitches[1] = RPU_DataRead(ADDRESS_U10_B);

   // Turn on Switch strobe 7 & Read Switches
   RPU_DataWrite(ADDRESS_U10_A, 0x80);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl & 0xF7);
   // Wait for switch capacitors to charge
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   DipSwitches[2] = RPU_DataRead(ADDRESS_U10_B);

   // Turn on U10 CB2 (strobe 8) and read switches
   RPU_DataWrite(ADDRESS_U10_A, 0x00);
   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl | 0x08);
   // Wait for switch capacitors to charge
   delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);
   DipSwitches[3] = RPU_DataRead(ADDRESS_U10_B);

   RPU_DataWrite(ADDRESS_U10_B_CONTROL, backupU10BControl);
   RPU_DataWrite(ADDRESS_U10_A, backupU10A);
}
#endif

void InitializeU11PIA() {
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
   // Store 9F in U11B Output
   RPU_DataWrite(ADDRESS_U11_B, DEFAULT_SOLENOID_STATE);
   CurrentSolenoidByte = DEFAULT_SOLENOID_STATE;
}

unsigned long RPU_TestPIAs() {
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
 *   Switch Handling Functions
 */

void RPU_PushToSwitchStack(uint8_t switchNumber) {
   SwitchStack.push(switchNumber);
}

uint8_t RPU_PullFirstFromSwitchStack() {
   return SwitchStack.pull();
}

bool RPU_ReadSingleSwitchState(uint8_t switchNum) {
   if (switchNum >= MAX_NUM_SWITCHES) {
      return false;
   }

   int switchByte = switchNum / 8;
   int switchBit = switchNum % 8;
   if (((SwitchesNow[switchByte]) >> switchBit) & 0x01) {
      return true;
   } else {
      return false;
   }
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

void RPU_SetupGameSwitches(int s_numSwitches, int s_numPrioritySwitches, const PlayfieldAndCabinetSwitch* s_gameSwitchArray) {
   NumGameSwitches = s_numSwitches;
   NumGamePrioritySwitches = s_numPrioritySwitches;
   GameSwitches = s_gameSwitchArray;
}

void RPU_ClearUpDownSwitchState() {
   // Bally/Stern does not have an up/down switch
   return;
}

bool RPU_GetUpDownSwitchState() {
   // Bally/Stern does not have an up/down switch
   return true;
}

/******************************************************
 *   Solenoid Handling Functions
 */

// int SpaceLeftOnSolenoidStack() {
//    return SolenoidStack.spaceLeft();
// }

void RPU_PushToSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes, bool disableOverride) {
   if (solenoidNumber >= RPU_NUM_SOLENOIDS) {
      return;
   }

   // if the solenoid stack is disabled and this isn't an override push, then return
   if (!disableOverride && !SolenoidStackEnabled) {
      return;
   }

   for (int count = 0; count < numPushes; count++) {
      if (!SolenoidStack.push(solenoidNumber)) {
         return; // Stack is full
      }
   }
}

void PushToFrontOfSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes) {
   // If the stack is full or disabled, return
   if (!SolenoidStackEnabled) {
      return;
   }

   for (int count = 0; count < numPushes; count++) {
      if (!SolenoidStack.pushFront(solenoidNumber)) {
         return; // Stack is full
      }
   }
}

uint8_t PullFirstFromSolenoidStack() {
   return SolenoidStack.pull();
}

bool RPU_PushToTimedSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes, unsigned long whenToFire, bool disableOverride) {
   uint8_t slot = TimedSolenoidStack.findFreeSlot();
   if (slot >= TimedSolenoidStack.getSize()) {
      return false;
   }
   TimedSolenoidStack.entries[slot].inUse = true;
   TimedSolenoidStack.entries[slot].pushTime = whenToFire;
   TimedSolenoidStack.entries[slot].disableOverride = disableOverride;
   TimedSolenoidStack.entries[slot].solenoidNumber = solenoidNumber;
   TimedSolenoidStack.entries[slot].numPushes = numPushes;
   return true;
}

void RPU_UpdateTimedSolenoidStack(unsigned long curTime) {
   for (int count = 0; count < TimedSolenoidStack.getSize(); count++) {
      if (TimedSolenoidStack.entries[count].inUse && TimedSolenoidStack.entries[count].pushTime < curTime) {
         RPU_PushToSolenoidStack(TimedSolenoidStack.entries[count].solenoidNumber, TimedSolenoidStack.entries[count].numPushes,
                                 TimedSolenoidStack.entries[count].disableOverride);
         TimedSolenoidStack.entries[count].inUse = false;
      }
   }
}

void RPU_SetCoinLockout(bool lockoutOff, uint8_t solbit) {
   if (!lockoutOff) {
      CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
   } else {
      CurrentSolenoidByte = CurrentSolenoidByte | solbit;
   }
   RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}

void RPU_SetDisableFlippers(bool disableFlippers, uint8_t solbit) {
   if (disableFlippers) {
      CurrentSolenoidByte = CurrentSolenoidByte | solbit;
   } else {
      CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
   }

   RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}

void RPU_SetContinuousSolenoidBit(bool bitOn, uint8_t solbit) {
   if (bitOn) {
      CurrentSolenoidByte = CurrentSolenoidByte | solbit;
   } else {
      CurrentSolenoidByte = CurrentSolenoidByte & ~solbit;
   }
   RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
}

bool RPU_FireContinuousSolenoid(uint8_t solBit, uint8_t numCyclesToFire) {
   if (NumCyclesBeforeRevertingSolenoidByte) {
      return false;
   }

   NumCyclesBeforeRevertingSolenoidByte = numCyclesToFire;

   RevertSolenoidBit = solBit;
   RPU_SetContinuousSolenoidBit(false, solBit);
   return true;
}

uint8_t RPU_ReadContinuousSolenoids() {
   return RPU_DataRead(ADDRESS_U11_B);
}

void RPU_DisableSolenoidStack() {
   SolenoidStackEnabled = false;
}

void RPU_EnableSolenoidStack() {
   SolenoidStackEnabled = true;
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
      if (digitNum) {
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

// This is confusing -
// Digit mask is like this
//   bit=   b7 b6 b5 b4 b3 b2 b1 b0
//   digit=  x  x  6  5  4  3  2  1
//   (with digit 6 being the least-significant, 1's digit
//
// so, looking at it from left to right on the display
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
   // A period of zero toggles display every other time
   if (period != 0) {
      if ((curTime / period) % 2) {
         RPU_SetDisplay(displayNumber, value, true, minDigits);
      } else {
         RPU_SetDisplayBlank(displayNumber, 0);
      }
   }
}

void RPU_SetDisplayFlashCredits(unsigned long curTime, int period) {
   if (period) {
      if ((curTime / period) % 2) {
         DisplayDigitEnable[4] |= 0x06;
      } else {
         DisplayDigitEnable[4] &= 0x39;
      }
   }
}

/******************************************************
 *   Lamp Handling Functions
 */

void RPU_SetDimDivisor(uint8_t level, uint8_t divisor) {
   if (level == 1) {
      DimDivisor1 = divisor;
   }
   if (level == 2) {
      DimDivisor2 = divisor;
   }
}

// left shift is iterative on Arduinos, so a bit array is surprisingly faster
static const uint8_t BitShiftValues[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

void RPU_SetLampState(int lampNum, bool lampState, uint8_t lampDim, int lampFlashPeriod) {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return;
   }
   uint8_t lampRow = lampNum % 8;
   uint8_t lampCol = lampNum / 8;
   uint8_t lampBit = BitShiftValues[lampRow];

   if (lampState) {
      int adjustedLampFlash = lampFlashPeriod / 50;

      if (lampFlashPeriod != 0 && adjustedLampFlash == 0) {
         adjustedLampFlash = 1;
      }
      if (adjustedLampFlash > 250) {
         adjustedLampFlash = 250;
      }

      // Only turn on the lamp if there's no flash, because if there's a flash
      // then the lamp will be turned on by the ApplyFlashToLamps function
      if (lampFlashPeriod == 0) {
         LampStates[lampCol] &= ~(lampBit);
      }
      LampFlashPeriod[lampNum] = adjustedLampFlash;
   } else {
      LampStates[lampCol] |= lampBit;
      LampFlashPeriod[lampNum] = 0;
   }

   if (lampDim & 0x01) {
      LampDim1[lampCol] |= lampBit;
   } else {
      LampDim1[lampCol] &= ~lampBit;
   }

   if (lampDim & 0x02) {
      LampDim2[lampCol] |= lampBit;
   } else {
      LampDim2[lampCol] &= ~lampBit;
   }
}

uint8_t RPU_ReadLampState(int lampNum) {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return 0x00;
   }
   uint8_t lampStateByte = LampStates[lampNum / 8];
   return (lampStateByte & (0x01 << (lampNum % 8))) ? 0 : 1;
}

uint8_t RPU_ReadLampDim(int lampNum) {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return 0x00;
   }
   uint8_t lampDim = 0;
   uint8_t lampDimByte = LampDim1[lampNum / 8];
   if (lampDimByte & (0x01 << (lampNum % 8))) {
      lampDim |= 1;
   }

   lampDimByte = LampDim2[lampNum / 8];
   if (lampDimByte & (0x01 << (lampNum % 8))) {
      lampDim |= 2;
   }

   return lampDim;
}

int RPU_ReadLampFlash(int lampNum) {
   if (lampNum >= RPU_MAX_LAMPS || lampNum < 0) {
      return 0;
   }

   return LampFlashPeriod[lampNum] * 50;
}

void RPU_ApplyFlashToLamps(unsigned long curTime) {
   int curLampByte = 0;
   uint8_t curLampBit = 0;
   int curLampNum = 0;

   for (curLampByte = 0; curLampByte < RPU_NUM_LAMP_BANKS; curLampByte++) {
      curLampBit = 0x01;
      for (uint8_t curBit = 0; curBit < 8; curBit++) {
         if (LampFlashPeriod[curLampNum] != 0) {
            unsigned long adjustedLampFlash = (unsigned long)LampFlashPeriod[curLampNum] * (unsigned long)50;
            if ((curTime / adjustedLampFlash) % 2) {
               LampStates[curLampByte] &= ~(curLampBit);
            } else {
               LampStates[curLampByte] |= (curLampBit);
            }
         }

         curLampBit *= 2;
         curLampNum += 1;
      }
   }
}

void RPU_FlashAllLamps(unsigned long curTime) {
   for (int count = 0; count < RPU_MAX_LAMPS; count++) {
      RPU_SetLampState(count, true, 0, 500);
   }

   RPU_ApplyFlashToLamps(curTime);
}

void RPU_TurnOffAllLamps() {
   for (int count = 0; count < RPU_MAX_LAMPS; count++) {
      RPU_SetLampState(count, false, 0, 0);
   }
}

/******************************************************
 *   Helper Functions
 */

void RPU_ClearVariables() {
   // Reset solenoid stack
   SolenoidStack.reset();

   // Reset switch stack
   SwitchStack.reset();

   CurrentDisplayDigit = 0;

   // Set default values for the displays
   for (int displayCount = 0; displayCount < 5; displayCount++) {
      for (int digitCount = 0; digitCount < RPU_OS_NUM_DIGITS; digitCount++) {
         DisplayDigits[displayCount][digitCount] = 0;
      }
      DisplayDigitEnable[displayCount] = 0x00;
   }

   // Turn off all lamp states
   for (int lampBankCounter = 0; lampBankCounter < RPU_NUM_LAMP_BANKS; lampBankCounter++) {
      LampStates[lampBankCounter] = 0xFF;
      LampDim1[lampBankCounter] = 0x00;
      LampDim2[lampBankCounter] = 0x00;
   }

   for (int lampFlashCount = 0; lampFlashCount < RPU_MAX_LAMPS; lampFlashCount++) {
      LampFlashPeriod[lampFlashCount] = 0;
   }

   // Reset all the switch values
   // (set them as closed so that if they're stuck they don't register as new events)
   uint8_t switchCount;
   for (switchCount = 0; switchCount < NUM_SWITCH_BYTES; switchCount++) {
      SwitchesMinus2[switchCount] = 0xFF;
      SwitchesMinus1[switchCount] = 0xFF;
      SwitchesNow[switchCount] = 0xFF;
   }

   TimedSolenoidStack.reset();

}

#if (RPU_OS_HARDWARE_REV >= 2 && defined(RPU_OS_USE_SB300))

void RPU_PlaySB300SquareWave(uint8_t soundRegister, uint8_t soundByte) {
   RPU_DataWrite(ADDRESS_SB300_SQUARE_WAVES + soundRegister, soundByte);
}

void RPU_PlaySB300Analog(uint8_t soundRegister, uint8_t soundByte) {
   RPU_DataWrite(ADDRESS_SB300_ANALOG + soundRegister, soundByte);
}

void RPU_InitSB300() {
   RPU_PlaySB300SquareWave(1, 0x00); // CR2: Timer 2 off, continuous, 16-bit, C2 clock, CR3 set
   RPU_PlaySB300SquareWave(0, 0x00); // CR3: Timer 3 off, continuous, 16-bit, C3 clock
   RPU_PlaySB300SquareWave(1, 0x01); // CR2: Timer 2 off, continuous, 16-bit, C2 clock, CR1 set
   RPU_PlaySB300SquareWave(0, 0x00); // CR1: Timer 1 off, continuous, 16-bit, C1 clock
}

void RPU_PlaySB300StartupBeep() {
   RPU_PlaySB300SquareWave(1, 0x92); // CR2: Timer 2 on, continuous, 16-bit, E clock, CR3 set
   RPU_PlaySB300SquareWave(0, 0x92); // CR3: Timer 3 on, continuous, 16-bit, E clock
   RPU_PlaySB300SquareWave(4, 0x02); // Timer 2 = 0x0200
   RPU_PlaySB300SquareWave(5, 0x00);
   RPU_PlaySB300SquareWave(6, 0x80); // Timer 3 = 0x8000
   RPU_PlaySB300SquareWave(7, 0x00);
   RPU_PlaySB300Analog(0, 0x02);
}

#endif

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

void RPU_PlayNativeChime(uint8_t soundByte) {
#if defined(RPU_OS_USE_SB100) && (RPU_OS_HARDWARE_REV == 2)
   RPU_PlaySB100Chime(soundByte);
#else
   RPU_PlayNativeSound(soundByte);
#endif
}

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


void InterruptService3() {
   const uint8_t u10AControl = RPU_DataRead(ADDRESS_U10_A_CONTROL);
   if (u10AControl & 0x80) {
      // self test switch
      if (RPU_DataRead(ADDRESS_U10_A_CONTROL) & 0x80) {
         SwitchStack.push(SW_SELF_TEST_SWITCH);
      }
      RPU_DataRead(ADDRESS_U10_A);
   }

   // If we get a weird interupt from U11B, clear it
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

      uint8_t u10BControlLatest = RPU_DataRead(ADDRESS_U10_B_CONTROL);

      // Backup contents of U10A
      uint8_t backup10A = RPU_DataRead(ADDRESS_U10_A);

      // Latch 0xFF separately without interrupt clear
      RPU_DataWrite(ADDRESS_U10_A, 0xFF);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);
      // Read U10B to clear interrupt
      RPU_DataRead(ADDRESS_U10_B);

      // Turn off U10BControl interrupts
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x30);

      // Copy old switch values
      for (uint8_t switchCount = 0; switchCount < NUM_SWITCH_BYTES; switchCount++) {
         SwitchesMinus2[switchCount] = SwitchesMinus1[switchCount];
         SwitchesMinus1[switchCount] = SwitchesNow[switchCount];

         // Enable switch strobe
         RPU_DataWrite(ADDRESS_U10_A, 0x01 << switchCount);

         // Turn off U10:CB2 if it's on (because it strobes the last bank of dip switches
         RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x34);

         // Delay for switch capacitors to charge
         delayMicroseconds(RPU_OS_SWITCH_DELAY_IN_MICROSECONDS);

         // Read the switches
         SwitchesNow[switchCount] = RPU_DataRead(ADDRESS_U10_B);

         // Unset the strobe
         RPU_DataWrite(ADDRESS_U10_A, 0x00);

         // Some switches need to trigger immediate closures (bumpers & slings)
         uint8_t startingClosures = (SwitchesNow[switchCount]) & (~SwitchesMinus1[switchCount]);
         bool immediateSolenoidFired = false;
         // If one of the switches is starting to close (off, on)
         if (startingClosures != 0) {
            // Loop on bits of switch uint8_t
            for (uint8_t bitCount = 0; bitCount < 8 && !immediateSolenoidFired; bitCount++) {
               // If this switch bit is closed
               if (startingClosures & 0x01) {
                  uint8_t startingSwitchNum = switchCount * 8 + bitCount;
                  // Loop on immediate switch data
                  for (int immediateSwitchCount = 0; immediateSwitchCount < NumGamePrioritySwitches && !immediateSolenoidFired;
                       immediateSwitchCount++) {
                     // If this switch requires immediate action
                     if (GameSwitches && startingSwitchNum == GameSwitches[immediateSwitchCount].switchNum) {
                        // Start firing this solenoid (just one until the closure is validate
                        PushToFrontOfSolenoidStack(GameSwitches[immediateSwitchCount].solenoid, 1);
                        immediateSolenoidFired = true;
                     }
                  }
               }
               startingClosures = startingClosures >> 1;
            }
         }

         immediateSolenoidFired = false;
         uint8_t validClosures = (SwitchesNow[switchCount] & SwitchesMinus1[switchCount]) & ~SwitchesMinus2[switchCount];
         // If there is a valid switch closure (off, on, on)
         if (validClosures) {
            // Loop on bits of switch uint8_t
            for (uint8_t bitCount = 0; bitCount < 8; bitCount++) {
               // If this switch bit is closed
               if (validClosures & 0x01) {
                  uint8_t validSwitchNum = switchCount * 8 + bitCount;
                  // Loop through all switches and see what's triggered
                  for (int validSwitchCount = 0; validSwitchCount < NumGameSwitches; validSwitchCount++) {
                     // If we've found a valid closed switch
                     if (GameSwitches && GameSwitches[validSwitchCount].switchNum == validSwitchNum) {
                        // If we're supposed to trigger a solenoid, then do it
                        if (GameSwitches[validSwitchCount].solenoid != SOL_NONE) {
                           if (validSwitchCount < NumGamePrioritySwitches && !immediateSolenoidFired) {
                              PushToFrontOfSolenoidStack(GameSwitches[validSwitchCount].solenoid,
                                                         GameSwitches[validSwitchCount].solenoidHoldTime);
                           } else {
                              RPU_PushToSolenoidStack(GameSwitches[validSwitchCount].solenoid,
                                                      GameSwitches[validSwitchCount].solenoidHoldTime);
                           }
                        } // End if this is a real solenoid
                     } // End if this is a switch in the switch table
                  } // End loop on switches in switch table
                  // Push this switch to the game rules stack
                  SwitchStack.push(validSwitchNum);
               }
               validClosures = validClosures >> 1;
            }
         }

         // There are no port reads or writes for the rest of the loop,
         // so we can allow the display interrupt to fire
         interrupts();

         // Wait so total delay will allow lamp SCRs to get to the proper voltage
         delayMicroseconds(RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS);

         noInterrupts();
      }
      RPU_DataWrite(ADDRESS_U10_A, backup10A);

      if (NumCyclesBeforeRevertingSolenoidByte != 0) {
         NumCyclesBeforeRevertingSolenoidByte -= 1;
         if (NumCyclesBeforeRevertingSolenoidByte == 0) {
            CurrentSolenoidByte |= RevertSolenoidBit;
            RevertSolenoidBit = 0x00;
         }
      }

#ifdef RPU_OS_USE_DASH32
      // mask out sound E line
      uint8_t curDisplayDigitEnableByte = RPU_DataRead(ADDRESS_U11_A);
      RPU_DataWrite(ADDRESS_U11_A, curDisplayDigitEnableByte | 0x02);
#endif

      // If we need to turn off momentary solenoids, do it first
      uint8_t momentarySolenoidAtStart = PullFirstFromSolenoidStack();
      if (momentarySolenoidAtStart != SOLENOID_STACK_EMPTY) {
         CurrentSolenoidByte = (CurrentSolenoidByte & 0xF0) | momentarySolenoidAtStart;
         RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
#ifdef RPU_OS_USE_DASH32
         // Raise CB2 so we don't unset the solenoid we just set
         RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x3C);
         // Mask off sound lines
         RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte | SOL_NONE);
         // Put CB2 back low
         RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);
         // Put solenoids back again
         RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
#endif
      } else {
         CurrentSolenoidByte = (CurrentSolenoidByte & 0xF0) | SOL_NONE;
         RPU_DataWrite(ADDRESS_U11_B, CurrentSolenoidByte);
      }

#ifdef RPU_OS_USE_DASH32
      // put back U11 A without E line
      RPU_DataWrite(ADDRESS_U11_A, curDisplayDigitEnableByte);
#endif

      for (int lampByteCount = 0; lampByteCount < 8; lampByteCount++) {
         for (uint8_t nibbleCount = 0; nibbleCount < 2; nibbleCount++) {
            // We skip iteration number 16 because the last position is to park the lamps
            if (lampByteCount == (7) && nibbleCount) {
               continue;
            }

            uint8_t lampData = 0xF0 + (lampByteCount * 2) + nibbleCount;

            interrupts();
            RPU_DataWrite(ADDRESS_U10_A, 0xFF);
            noInterrupts();

            // Latch address & strobe
            RPU_DataWrite(ADDRESS_U10_A, lampData);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE
            delayMicroseconds(2);
#endif

            RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x38);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE
            delayMicroseconds(2);
#endif

            RPU_DataWrite(ADDRESS_U10_B_CONTROL, 0x30);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE
            delayMicroseconds(2);
#endif

            // Use the inhibit lines to set the actual data to the lamp SCRs
            // (here, we don't care about the lower nibble because the address was already latched)
            uint8_t nibbleOffset = (nibbleCount != 0) ? 1 : 16;
            uint8_t lampOutput = (LampStates[lampByteCount] * nibbleOffset);
            // Every other time through the cycle, we OR in the dim variable
            // in order to dim those lights
            if (numberOfU10Interrupts % DimDivisor1) {
               lampOutput |= (LampDim1[lampByteCount] * nibbleOffset);
            }
            if (numberOfU10Interrupts % DimDivisor2) {
               lampOutput |= (LampDim2[lampByteCount] * nibbleOffset);
            }

            RPU_DataWrite(ADDRESS_U10_A, lampOutput | 0x0F);
#ifdef RPU_SLOW_DOWN_LAMP_STROBE
            delayMicroseconds(2);
#endif
         } // end loop on nibble
      } // end loop on lamp bytes

#ifdef RPU_OS_USE_AUX_LAMPS
      // Latch 0xFF separately without interrupt clear
      // to park 0xFF in main lamp board
      RPU_DataWrite(ADDRESS_U10_A, 0xFF);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

      // For the first four bits of lamps, we're going to look at LampStates[7] again
      // and use those top 4 bits that we didn't use before. Then we're going
      // to move on with bytes 8, 9, and 10 for the remaining 24 bits of data
      uint8_t auxBankNum = 0;
      for (int lampByteCount = 7; lampByteCount < RPU_NUM_LAMP_BANKS; lampByteCount++) {
         for (uint8_t nibbleCount = 0; nibbleCount < 2; nibbleCount++) {
            if (lampByteCount == 7) {
               nibbleCount = 1; // skip the first nibble of uint8_t 7 because it belongs to primary lamps
            }
            uint8_t nibbleOffset = (nibbleCount != 0) ? 1 : 16;
            uint8_t lampOutput = (LampStates[lampByteCount] * nibbleOffset);
            // Every other time through the cycle, we OR in the dim variable
            // in order to dim those lights
            if (numberOfU10Interrupts % DimDivisor1) {
               lampOutput |= (LampDim1[lampByteCount] * nibbleOffset);
            }
            if (numberOfU10Interrupts % DimDivisor2) {
               lampOutput |= (LampDim2[lampByteCount] * nibbleOffset);
            }

            // The data will be in the upper nibble, but we need the bank count in the lower
            lampOutput &= 0xF0;
            lampOutput += auxBankNum;

            interrupts();
            RPU_DataWrite(ADDRESS_U10_A, 0xFF);
            noInterrupts();

            RPU_DataWrite(ADDRESS_U10_A, lampOutput | 0xF0);
            RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) | 0x08);
            RPU_DataWrite(ADDRESS_U11_A_CONTROL, RPU_DataRead(ADDRESS_U11_A_CONTROL) & 0xF7);
            RPU_DataWrite(ADDRESS_U10_A, lampOutput);

            auxBankNum += 1;
         }
      }
#endif

      // Latch 0xFF separately without interrupt clear
      RPU_DataWrite(ADDRESS_U10_A, 0xFF);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) | 0x08);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, RPU_DataRead(ADDRESS_U10_B_CONTROL) & 0xF7);

      interrupts();
      noInterrupts();

      InsideZeroCrossingInterrupt = 0;
      RPU_DataWrite(ADDRESS_U10_A, backup10A);
      RPU_DataWrite(ADDRESS_U10_B_CONTROL, u10BControlLatest);

      // Read U10B to clear interrupt
      RPU_DataRead(ADDRESS_U10_B);
      numberOfU10Interrupts += 1;
   }
}

void RPU_HookInterrupts() {
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

bool LookFor6800Activity() {
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

void SetupArduinoPorts() {
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

   RPU_ApplyFlashToLamps(currentTime);
   RPU_UpdateTimedSolenoidStack(currentTime);
}

// This function should eventually support auto-detect and initialize the appropriate
// ISRs for the detected architecture.
unsigned long RPU_InitializeMPU(unsigned long initOptions, uint8_t creditResetSwitch) {
   unsigned long retVal = 0;

   // Wait for board to boot
   delayMicroseconds(50000);
   delayMicroseconds(50000);

#if (RPU_OS_HARDWARE_REV == 1) or (RPU_OS_HARDWARE_REV == 2)
   (void)creditResetSwitch;

   if ((initOptions & (RPU_CMD_BOOT_ORIGINAL | RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET |
                      RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED | RPU_CMD_AUTODETECT_ARCHITECTURE)) != 0) {
      retVal |= RPU_RET_OPTION_NOT_SUPPORTED;
   }

   if (LookFor6800Activity()) {
      if ((initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) != 0) {
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

   if ((initOptions &
       (RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET | RPU_CMD_AUTODETECT_ARCHITECTURE)) != 0) {
      retVal |= RPU_RET_OPTION_NOT_SUPPORTED;
   }

   pinMode(13, INPUT);
   bool switchStateClosed = digitalRead(13) == 0;
   bool bootToOriginal = false;

   if (switchStateClosed) {
      retVal |= RPU_RET_SELECTOR_SWITCH_ON;
   }

   if ((initOptions & RPU_CMD_BOOT_ORIGINAL) != 0 || (switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED) != 0) ||
       (!switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED) != 0)) {
      bootToOriginal = true;
   }

   if (bootToOriginal) {
      RPU_DEBUG_MESSAGE("* Asked to boot to original\n");
      RPU_DEBUG_DELAY(100);

      // Let the 680X run
      pinMode(14, OUTPUT); // Halt
      digitalWrite(14, HIGH);
      if ((initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) != 0) {
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
   if ((initOptions & RPU_CMD_BOOT_ORIGINAL) != 0 || (switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED) != 0) ||
       (!switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED) != 0) ||
       (creditResetButtonHit && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET) != 0) ||
       (!creditResetButtonHit && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET) != 0)) {
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
      if ((initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) == 0) {
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
   ReadDipSwitches();
#endif

#if (RPU_OS_HARDWARE_REV == 4) || (RPU_OS_HARDWARE_REV > 100)
   pinMode(RPU_DIAGNOSTIC_PIN, INPUT);
   if (digitalRead(RPU_DIAGNOSTIC_PIN) == 1) {
      retVal |= RPU_RET_DIAGNOSTIC_REQUESTED;
   }
#endif

   // Reset address bus
   RPU_DataRead(0);
   RPU_ClearVariables();

   RPU_DEBUG_MESSAGE("* About to hook interrupts\n");
   RPU_DEBUG_DELAY(100);

   RPU_HookInterrupts();
   RPU_DataRead(0); // Reset address bus

   // Clear all possible interrupts by reading the registers
   RPU_DataRead(ADDRESS_U11_A);
   RPU_DataRead(ADDRESS_U11_B);
   RPU_DataRead(ADDRESS_U10_A);
   RPU_DataRead(ADDRESS_U10_B);
   if ((initOptions & RPU_CMD_PERFORM_MPU_TEST) != 0) {
      retVal |= RPU_TestPIAs();
   }
   RPU_DataRead(0); // Reset address bus

   return retVal;
}
