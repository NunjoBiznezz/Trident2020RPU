#if (RPU_OS_HARDWARE_REV == 1) || (RPU_OS_HARDWARE_REV == 2)

#if !defined(__AVR_ATmega328P__)
#error "Hardware revisions 1 & 2 require ATmega328P, check your compiler settings"
#endif

#include "RPU.h"
#include "RPU_Internal.h"
#include <Arduino.h>

// Rev 1/2 pin map (Arduino Nano 328P on J5):
//   A0-A4 = MPU A0,A1,A3,A4,A7   A5 = VMA
//   D5-D7 = data bits 0-2         D8-D12 = data bits 3-7
//   D3 = R/W                      D4 = CLK (PHI2)

void RPU_DataWrite(int address, uint8_t data) {
   DDRD = DDRD | 0xE8;
   DDRB = DDRB | 0x1F;

   PORTD = (PORTD & 0xF7); // R/W LOW

   PORTD = (PORTD & 0x1F) | ((data & 0x07) << 5); // data bits 0-2 on D5-D7
   PORTB = (PORTB & 0xE0) | (data >> 3);           // data bits 3-7 on D8-D12

   PORTC = (PORTC & 0xE0) | address;

   while ((PIND & 0x10))   // wait for falling clock edge
      ;
   PORTC = PORTC | 0x20;   // VMA ON
   while (!(PIND & 0x10))  // wait clock low
      ;
   while ((PIND & 0x10))   // wait clock high
      ;
   while (!(PIND & 0x10))  // wait clock low
      ;
   PORTC = PORTC & 0xDF;   // VMA OFF
   PORTC = PORTC & 0xE0;   // clear address lines
   PORTD = (PORTD | 0x08); // R/W HIGH

   DDRD = DDRD & 0x1F;
   DDRB = DDRB & 0xE0;
}

uint8_t RPU_DataRead(int address) {
   DDRD = DDRD & 0x1F;
   DDRB = DDRB & 0xE0;

   DDRD = DDRD | 0x08;
   PORTD = (PORTD | 0x08); // R/W HIGH

   PORTC = (PORTC & 0xE0) | address;

   while ((PIND & 0x10))   // wait for falling clock edge
      ;
   PORTC = PORTC | 0x20;   // VMA ON
   while (!(PIND & 0x10))
      ;
   while ((PIND & 0x10))
      ;
   while (!(PIND & 0x10))
      ;

   uint8_t inputData = (PIND >> 5) | (PINB << 3);

   PORTC = PORTC & 0xDF;   // VMA OFF
   PORTD = (PORTD & 0xF7); // R/W LOW
   PORTC = (PORTC & 0xE0);

   return inputData;
}

static bool LookFor6800Activity() {
   unsigned long startTime = millis();
   bool sawHigh = false;
   bool sawLow = false;
   while ((millis() - startTime) < 1000) {
      if (PINC & 0x20) {
         sawHigh = true;
      } else {
         sawLow = true;
      }
   }
   return sawHigh && sawLow;
}

void RPU_HW_SetupPorts(uint16_t &) {
#if (RPU_OS_HARDWARE_REV == 1)
   // A0-A4 are address lines; A5 is VMA (output)
   DDRC = DDRC | 0x3F;
   DDRB = DDRB | 0x20; // D13 = address line A5 (set low)
   PORTB = PORTB & 0xDF;
   DDRD = DDRD & 0xEB;
   DDRD = DDRD | 0xE8;
   PORTC = PORTC & 0xDF; // VMA OFF
   PORTD = (PORTD | 0x08); // R/W HIGH
#elif (RPU_OS_HARDWARE_REV == 2)
   DDRC = DDRC | 0x3F;
   DDRB = DDRB | 0x20; // D13 = address line A7 (set high)
   PORTB = PORTB | 0x20;
   DDRD = DDRD & 0xEB;
   DDRD = DDRD | 0xE8;
   PORTC = PORTC & 0xDF; // VMA OFF
   PORTD = (PORTD | 0x08); // R/W HIGH
#endif
}

bool RPU_InitializeBSP(uint16_t initOptions, uint8_t, uint16_t &retVal) {
   if (initOptions & (RPU_CMD_BOOT_ORIGINAL | RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET |
                      RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED |
                      RPU_CMD_AUTODETECT_ARCHITECTURE)) {
      retVal |= RPU_RET_OPTION_NOT_SUPPORTED;
   }

   if (LookFor6800Activity()) {
      if (initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) {
         retVal |= RPU_RET_ORIGINAL_CODE_REQUESTED;
         return true;
      } else {
         while (1)
            ;
      }
   }
   return false;
}

#endif // RPU_OS_HARDWARE_REV 1 or 2