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

#pragma once

#include <stdint.h>

// Internal methods needed by controllers
extern void RPU_DataWrite(int address, uint8_t data);
extern uint8_t RPU_DataRead(int address);

#if (RPU_OS_HARDWARE_REV == 1)
constexpr uint16_t ADDRESS_U10_A = 0x14;
constexpr uint16_t ADDRESS_U10_A_CONTROL = 0x15;
constexpr uint16_t ADDRESS_U10_B = 0x16;
constexpr uint16_t ADDRESS_U10_B_CONTROL = 0x17;
constexpr uint16_t ADDRESS_U11_A = 0x18;
constexpr uint16_t ADDRESS_U11_A_CONTROL = 0x19;
constexpr uint16_t ADDRESS_U11_B = 0x1A;
constexpr uint16_t ADDRESS_U11_B_CONTROL = 0x1B;
constexpr uint16_t ADDRESS_SB100 = 0x10;

#elif (RPU_OS_HARDWARE_REV == 2)

constexpr uint16_t ADDRESS_U10_A = 0x00;
constexpr uint16_t ADDRESS_U10_A_CONTROL = 0x01;
constexpr uint16_t ADDRESS_U10_B = 0x02;
constexpr uint16_t ADDRESS_U10_B_CONTROL = 0x03;
constexpr uint16_t ADDRESS_U11_A = 0x08;
constexpr uint16_t ADDRESS_U11_A_CONTROL = 0x09;
constexpr uint16_t ADDRESS_U11_B = 0x0A;
constexpr uint16_t ADDRESS_U11_B_CONTROL = 0x0B;
constexpr uint16_t ADDRESS_SB100 = 0x10;
constexpr uint16_t ADDRESS_SB100_CHIMES = 0x18;
constexpr uint16_t ADDRESS_SB300_SQUARE_WAVES = 0x10;
constexpr uint16_t ADDRESS_SB300_ANALOG = 0x18;

#elif (RPU_OS_HARDWARE_REV == 3) || (RPU_OS_HARDWARE_REV == 4)

constexpr uint16_t ADDRESS_U10_A = 0x88;
constexpr uint16_t ADDRESS_U10_A_CONTROL = 0x89;
constexpr uint16_t ADDRESS_U10_B = 0x8A;
constexpr uint16_t ADDRESS_U10_B_CONTROL = 0x8B;
constexpr uint16_t ADDRESS_U11_A = 0x90;
constexpr uint16_t ADDRESS_U11_A_CONTROL = 0x91;
constexpr uint16_t ADDRESS_U11_B = 0x92;
constexpr uint16_t ADDRESS_U11_B_CONTROL = 0x93;
constexpr uint16_t ADDRESS_SB100 = 0xA0;
constexpr uint16_t ADDRESS_SB100_CHIMES = 0xC0;
constexpr uint16_t ADDRESS_SB300_SQUARE_WAVES = 0xA0;
constexpr uint16_t ADDRESS_SB300_ANALOG = 0xC0;

#elif (RPU_OS_HARDWARE_REV >= 100)
constexpr uint16_t ADDRESS_U10_A = 0x88;
constexpr uint16_t ADDRESS_U10_A_CONTROL = 0x89;
constexpr uint16_t ADDRESS_U10_B = 0x8A;
constexpr uint16_t ADDRESS_U10_B_CONTROL = 0x8B;
constexpr uint16_t ADDRESS_U11_A = 0x90;
constexpr uint16_t ADDRESS_U11_A_CONTROL = 0x91;
constexpr uint16_t ADDRESS_U11_B = 0x92;
constexpr uint16_t ADDRESS_U11_B_CONTROL = 0x93;
constexpr uint16_t ADDRESS_SB100 = 0xA0;
constexpr uint16_t ADDRESS_SB100_CHIMES = 0xC0;
constexpr uint16_t ADDRESS_SB300_SQUARE_WAVES = 0xA0;
constexpr uint16_t ADDRESS_SB300_ANALOG = 0xC0;

#endif



#if !defined(RPU_DEBUG_MESSAGES)
#define RPU_DEBUG_MESSAGES 0
#define RPU_DEBUG_MESSAGE(msg)
#define RPU_DEBUG_DELAY(ms)
#define RPU_DEBUG_PRINTF(...)
#else
#define RPU_DEBUG_MESSAGE(msg) Serial.write(msg);
#define RPU_DEBUG_DELAY(ms) delay(ms)
#define RPU_DEBUG_PRINTF(...)           \
{                                 \
char _debug_buf[128];             \
sprintf(_debug_buf, __VA_ARGS__); \
Serial.write(_debug_buf);         \
}
#endif
