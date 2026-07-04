#if (RPU_OS_HARDWARE_REV == 3)

#if !defined(__AVR_ATmega2560__)
#error "RPU_OS_HARDWARE_REV 3 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#include "RPU.h"
#include "RPU_Internal.h"
#include <Arduino.h>

// Rev 3 pin map (MEGA 2560 Pro on J5):
//   D2=IRQ  D3=CLK  D4=VMA  D5=R/W  D6-D12=D0-D6  D13=SWITCH  D14=HALT  D15=D7  D16-D30=A0-A14

void RPU_DataWrite(int address, uint8_t data) {
   DDRH = DDRH | 0x78;
   DDRB = DDRB | 0x70;
   DDRJ = DDRJ | 0x01;

   PORTE = (PORTE & 0xF7); // R/W LOW

   PORTH = (PORTH & 0x87) | ((data & 0x0F) << 3); // data bits 0-3 on H3-H6
   PORTB = (PORTB & 0x8F) | ((data & 0x70));       // data bits 4-6 on B4-B6
   PORTJ = (PORTJ & 0xFE) | (data >> 7);           // data bit 7 on J0

   PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
   PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) |
           ((address & 0x0010) >> 3) | ((address & 0x0020) >> 5); // A2-A5
   PORTA = ((address & 0x3FC0) >> 6);                             // A6-A13
   PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

   while ((PINE & 0x20))  // wait for falling clock edge
      ;
   PORTG = PORTG | 0x20; // VMA ON
   while (!(PINE & 0x20))
      ;
   while ((PINE & 0x20))
      ;
   while (!(PINE & 0x20))
      ;
   PORTG = PORTG & 0xDF; // VMA OFF

   PORTH = (PORTH & 0xFC);
   PORTD = (PORTD & 0xF0);
   PORTA = 0;
   PORTC = (PORTC & 0x3F);

   PORTE = (PORTE | 0x08); // R/W HIGH

   DDRH = DDRH & 0x87;
   DDRB = DDRB & 0x8F;
   DDRJ = DDRJ & 0xFE;
}

uint8_t RPU_DataRead(int address) {
   DDRH = DDRH & 0x87;
   DDRB = DDRB & 0x8F;
   DDRJ = DDRJ & 0xFE;

   DDRE = DDRE | 0x08;
   PORTE = (PORTE | 0x08); // R/W HIGH

   PORTH = (PORTH & 0xFC) | ((address & 0x0001) << 1) | ((address & 0x0002) >> 1); // A0-A1
   PORTD = (PORTD & 0xF0) | ((address & 0x0004) << 1) | ((address & 0x0008) >> 1) |
           ((address & 0x0010) >> 3) | ((address & 0x0020) >> 5); // A2-A5
   PORTA = ((address & 0x3FC0) >> 6);                             // A6-A13
   PORTC = (PORTC & 0x3F) | ((address & 0x4000) >> 7) | ((address & 0x8000) >> 9); // A14-A15

   while ((PINE & 0x20))  // wait for falling clock edge
      ;
   PORTG = PORTG | 0x20; // VMA ON
   while (!(PINE & 0x20))
      ;
   while ((PINE & 0x20))
      ;
   while (!(PINE & 0x20))
      ;

   uint8_t inputData;
   inputData = (PINH & 0x78) >> 3;
   inputData |= (PINB & 0x70);
   inputData |= PINJ << 7;

   PORTG = PORTG & 0xDF; // VMA OFF
   PORTE = (PORTE & 0xF7); // R/W LOW

   PORTH = (PORTH & 0xFC);
   PORTD = (PORTD & 0xF0);
   PORTA = 0;
   PORTC = (PORTC & 0x3F);

   return inputData;
}

void RPU_HW_SetupPorts(uint16_t &) {
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
      pinMode(count, OUTPUT); // address lines
   }
   digitalWrite(5, HIGH); // R/W high (Read)
   digitalWrite(4, LOW);  // VMA low
}

bool RPU_HW_EarlyInit(uint16_t initOptions, uint8_t, uint16_t &retVal) {
   RPU_DEBUG_MESSAGE("* Starting Setup for Rev 3\n");

   if (initOptions & (RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET |
                      RPU_CMD_AUTODETECT_ARCHITECTURE)) {
      retVal |= RPU_RET_OPTION_NOT_SUPPORTED;
   }

   pinMode(13, INPUT);
   bool switchStateClosed = digitalRead(13) ? false : true;

   if (switchStateClosed) {
      retVal |= RPU_RET_SELECTOR_SWITCH_ON;
   }

   bool bootToOriginal = (initOptions & RPU_CMD_BOOT_ORIGINAL) ||
                         (switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED)) ||
                         (!switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED));

   if (bootToOriginal) {
      RPU_DEBUG_MESSAGE("* Asked to boot to original\n");
      RPU_DEBUG_DELAY(100);
      pinMode(14, OUTPUT);
      digitalWrite(14, HIGH); // release HALT
      if (initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN) {
         retVal |= RPU_RET_ORIGINAL_CODE_REQUESTED;
         return true;
      } else {
         while (1)
            ;
      }
   }

   pinMode(14, OUTPUT);
   digitalWrite(14, LOW); // assert HALT to keep 680X off the bus
   return false;
}

#endif // RPU_OS_HARDWARE_REV 3