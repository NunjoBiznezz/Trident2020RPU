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
#if (RPU_MPU_ARCHITECTURE<10)
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
#else
constexpr uint16_t PIA_DISPLAY_PORT_A = 0x2800;
constexpr uint16_t PIA_DISPLAY_CONTROL_A = 0x2801;
constexpr uint16_t PIA_DISPLAY_PORT_B = 0x2802;
constexpr uint16_t PIA_DISPLAY_CONTROL_B = 0x2803;
constexpr uint16_t PIA_SWITCH_PORT_A = 0x3000;
constexpr uint16_t PIA_SWITCH_CONTROL_A = 0x3001;
constexpr uint16_t PIA_SWITCH_PORT_B = 0x3002;
constexpr uint16_t PIA_SWITCH_CONTROL_B = 0x3003;
constexpr uint16_t PIA_LAMPS_PORT_A = 0x2400;
constexpr uint16_t PIA_LAMPS_CONTROL_A = 0x2401;
constexpr uint16_t PIA_LAMPS_PORT_B = 0x2402;
constexpr uint16_t PIA_LAMPS_CONTROL_B = 0x2403;
constexpr uint16_t PIA_SOLENOID_PORT_A = 0x2200;
constexpr uint16_t PIA_SOLENOID_CONTROL_A = 0x2201;
constexpr uint16_t PIA_SOLENOID_PORT_B = 0x2202;
constexpr uint16_t PIA_SOLENOID_CONTROL_B = 0x2203;
#if (RPU_MPU_ARCHITECTURE==13)
constexpr uint16_t PIA_SOUND_COMMA_PORT_A = 0x2100;
constexpr uint16_t PIA_SOUND_COMMA_CONTROL_A = 0x2101;
constexpr uint16_t PIA_SOUND_COMMA_PORT_B = 0x2102;
constexpr uint16_t PIA_SOUND_COMMA_CONTROL_B = 0x2103;
#endif
#if (RPU_MPU_ARCHITECTURE==15)
constexpr uint16_t PIA_SOUND_11_PORT_A = 0x2100;
constexpr uint16_t PIA_SOUND_11_CONTROL_A = 0x2101;
constexpr uint16_t PIA_SOLENOID_11_PORT_B = 0x2102;
constexpr uint16_t PIA_SOLENOID_11_CONTROL_B = 0x2103;
constexpr uint16_t PIA_ALPHA_DISPLAY_PORT_A = 0x2C00;
constexpr uint16_t PIA_ALPHA_DISPLAY_CONTROL_A = 0x2C01;
constexpr uint16_t PIA_ALPHA_DISPLAY_PORT_B = 0x2C02;
constexpr uint16_t PIA_ALPHA_DISPLAY_CONTROL_B = 0x2C03;
constexpr uint16_t PIA_NUM_DISPLAY_PORT_A = 0x3400;
constexpr uint16_t PIA_NUM_DISPLAY_CONTROL_A = 0x3401;
constexpr uint16_t PIA_WIDGET_PORT_B = 0x3402;
constexpr uint16_t PIA_WIDGET_CONTROL_B = 0x3403;
#endif
#endif

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


// Internal methods needed by controllers
extern void RPU_DataWrite(int address, uint8_t data);
extern uint8_t RPU_DataRead(int address);

