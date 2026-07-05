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




#if RPU_MPU_ARCH_IS_BSOS()
#include "bsp/bsp_bsos.h"
#elif RPU_MPU_ARCH_IS_WMS()
#include "bsp/bsp_wms.h"
#else
#error "Unsupported MPU"
#endif


#if defined(RPU_DEBUG_MESSAGES) && defined(DEBUG_PORT)
#  define RPU_DEBUG_MESSAGE(msg)    DEBUG_PORT.write(msg)
#  define RPU_DEBUG_DELAY(ms)       delay(ms)
#  define RPU_DEBUG_PRINTF(...)     { char _debug_buf[128]; sprintf(_debug_buf, __VA_ARGS__); DEBUG_PORT.print(_debug_buf); }
#else
#  define RPU_DEBUG_MESSAGE(msg)
#  define RPU_DEBUG_DELAY(ms)
#  define RPU_DEBUG_PRINTF(...)
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
