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

#include "RPU_config.h"
#include <stdint.h>


// Use the precompiler variables to compute build parameters
#define RPU_MACHINE_FAMILY_BALLY_STERN  1000

#define RPU_MACHINE_FAMILY_WILLIAMS     2000
#define RPU_MPU_ARCH_WMS_SYSTEM3        10
#define RPU_MPU_ARCH_WMS_SYSTEM4_6      11
#define RPU_MPU_ARCH_WMS_SYSTEM7        13
#define RPU_MPU_ARCH_WMS_SYSTEM11       15

// Define the machine architecture based on the MPU architecture
#if (RPU_MPU_ARCHITECTURE<10)
#define RPU_MACHINE_FAMILY RPU_MACHINE_FAMILY_BALLY_STERN
#else
#define RPU_MACHINE_FAMILY RPU_MACHINE_FAMILY_WILLIAMS
#endif


#if (RPU_MACHINE_FAMILY == RPU_MACHINE_FAMILY_BALLY_STERN)
#include "bsp/bsp_bsos.h"
#elif (RPU_MACHINE_FAMILY == RPU_MACHINE_FAMILY_WILLIAMS)
#include "bsp/bsp_wms.h"
#else
#error "Unsupported machine family"
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


// Low-level bus access — defined in bsp/hw_rev*.cpp, one per hardware revision
extern void RPU_DataWrite(int address, uint8_t data);
extern uint8_t RPU_DataRead(int address);

// Hardware-revision init hooks — called from RPU_InitializeMPU in RPU.cpp
// RPU_InitializeBSP: performs rev-specific pin setup and boot-to-original logic.
// Returns a uint16_t value indicating the initialization status.
extern uint16_t RPU_InitializeBSP(uint16_t initOptions, uint8_t creditResetSwitch=0xff);

// RPU_HW_SetupPorts: called after RPU_InitializeBSP for any remaining port config
//   (e.g. diagnostic pin check). May set bits in retVal.
extern void RPU_HW_SetupPorts(uint16_t &retVal);

// PIA helper used by hw_rev4 and hw_rev101_102 during boot-to-original detection
extern bool CheckCreditResetSwitchArch1(uint8_t creditResetSwitch);
