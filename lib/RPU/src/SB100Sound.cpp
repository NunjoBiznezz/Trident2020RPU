#include "RPU_config.h"
#include "RPU.h"
#include <Arduino.h>

extern void RPU_DataWrite(int address, uint8_t data);

// With hardware rev 1, this function relies on D13 being connected to A5 because it writes to address 0xA0
// A0  - A0   0
// A1  - A1   0
// A2  - n/c  0
// A3  - A2   0
// A4  - A3   0
// A5  - D13  1
// A6  - n/c  0
// A7  - A4   1
// A8  - n/c  0
// A9  - GND  0
// A10 - n/c  0
// A11 - n/c  0
// A12 - GND  0
// A13 - n/c  0
#ifdef RPU_OS_USE_SB100
void RPU_PlaySB100(uint8_t soundByte) {
#if (RPU_OS_HARDWARE_REV == 1)
   PORTB = PORTB | 0x20;
#endif

   RPU_DataWrite(ADDRESS_SB100, soundByte);

#if (RPU_OS_HARDWARE_REV == 1)
   PORTB = PORTB & 0xDF;
#endif
}

#if (RPU_OS_HARDWARE_REV == 2)
void RPU_PlaySB100Chime(uint8_t soundByte) {
   RPU_DataWrite(ADDRESS_SB100_CHIMES, soundByte);
}
#endif
#endif
