/**************************************************************************
 *     This file is part of the RPU OS for Arduino Project.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 6/1/2020

    RPU OS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    RPU OS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>

/***
  Use this file to set game-specific and hardware-specific parameters
***/

// Plug-in board architectures (0-99 is for J5, 100-199 is for CPU socket)
// Hardware Rev 1 generally uses an Arduino Nano & (optional) 74125
// Hardware Rev 2 uses an Arduino Nano, a 74155, and a 74240
// Hardware Rev 3 uses a MEGA 2560 Pro, and nothing else
// Hardware Rev 4 uses a MEGA 2560 Pro (all the pins) on a larger board (display & WIFI)
// Hardware Rev 100 (different order of magnitude because it's a different approach) plugs into the CPU socket
// Hardware Rev 101 - first RPU CPU interposer release board
// Hardware Rev 102 - second RPU (with display and WIFI socket)
#if !defined(RPU_OS_HARDWARE_REV)
#error "RPU_OS_HARDWARE_REV not defined. Please define it in platformio.ini or in RPU_config.h"
#endif

#define RPU_OS_HARDWARE_REV_IS(rev)        (RPU_OS_HARDWARE_REV == (rev))


#define RPU_MPU_ARCHITECTURE_BSOS           1
#define RPU_MPU_ARCHITECTURE_SYSTEM3        10
#define RPU_MPU_ARCHITECTURE_SYSTEM4_6      11
#define RPU_MPU_ARCHITECTURE_SYSTEM7        13
#define RPU_MPU_ARCHITECTURE_SYSTEM11       15

// Available Architectures (0-9 is for B/S Boards, 10-19 is for W)
//  RPU_MPU_ARCHITECTURE 1 = -17, -35, 100, 200, or compatible
//  RPU_MPU_ARCHITECTURE 11 = Sys 4, 6
//  RPU_MPU_ARCHITECTURE 13 = Sys 7
//  RPU_MPU_ARCHITECTURE 15 = Sys 11
#if !defined(RPU_MPU_ARCHITECTURE)
#error "RPU_MPU_ARCHITECTURE not defined. Please define it in platformio.ini or in RPU_config.h"
#endif

#define RPU_MPU_ARCH_IS(arch)    (RPU_MPU_ARCHITECTURE == (arch))
#define RPU_MPU_ARCH_IS_BSOS()   (RPU_MPU_ARCHITECTURE < 10)
#define RPU_MPU_ARCH_IS_WMS()    (RPU_MPU_ARCHITECTURE >= 10)


// Some boards will assume a 6800 is the processor (RPU_OS_HARDWARE_REV 1 through 4)
// and some boards will try to detect the processor (RPU_OS_HARDWARE_REV 102)
// but in other cases we can specify if we're building for a 6800.
// Define RPU_MPU_BUILD_FOR_6800 with a 0 for 6802 or 6808, and with
// a 1 for 6800
#if !defined(RPU_MPU_BUILD_FOR_6800)
#error "RPU_MPU_BUILD_FOR_6800 not defined. Please define it in platformio.ini or in RPU_config.h"
#endif

// Enforce mutual exclusivity: at most one native sound card may be active per build.
#if (defined(RPU_OS_USE_SB100) + defined(RPU_OS_USE_SB300) + defined(RPU_OS_USE_DASH51) + defined(RPU_OS_USE_S_AND_T) + defined(RPU_OS_USE_DASH32)) > 1
#  error "Only one native sound card may be defined per build (RPU_OS_USE_SB100 / RPU_OS_USE_SB300 / RPU_OS_USE_DASH51 / RPU_OS_USE_S_AND_T). Check platformio.ini or CMake options."
#endif

// These defines allow this configuration to eliminate some functions
// to reduce program size
// Note: These are now controlled by CMake build options. Do not define them here.
// They will be passed as compile definitions by the build system.
// Available options:
//   RPU_OS_USE_DIP_SWITCHES - Enable DIP switch reading
//   RPU_OS_USE_SB100 - Enable Stern SB-100 sound card support
//   RPU_OS_USE_SB300 - Enable Stern SB-300 sound card support (requires hardware rev >= 2)
//   RPU_OS_USE_WAV_TRIGGER - Enable WavTrigger 1.3 support
//   RPU_OS_USE_7_DIGIT_DISPLAYS - Enable 7-digit displays
//   RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS - Use 6-digit credit with 7-digit displays
//   RPU_OS_USE_AUX_LAMPS - Enable auxiliary lamps (88 lamps instead of 60)
//   RPU_OS_USE_S_AND_T - Enable S&T sound card support
//   RPU_OS_USE_DASH51 - Enable Dash-51 sound card support
//   RPU_OS_DISABLE_CPC_FOR_SPACE - Disable CPC code to save space

// Fast boards might need a slower lamp zeroCrossingISR
// #define RPU_OS_SLOW_DOWN_LAMP_STROBE  0

#define RPU_USES_SB300()         (defined(RPU_OS_USE_SB300) && (RPU_OS_HARDWARE_REV_IS(3) || RPU_OS_HARDWARE_REV_IS(4))

