//
// Created by Douglas Bercot on 6/24/26.
//
#include "RPU_config.h"
#include "RPU_Core.h"
#include "RPU_Internal.h"

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