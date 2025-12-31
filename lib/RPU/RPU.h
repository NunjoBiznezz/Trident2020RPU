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

#ifndef RPU_OS_H

#include "RPU_config.h" // include this here for safety sake
#include <stdint.h>

#define RPU_OS_MAJOR_VERSION 5
#define RPU_OS_MINOR_VERSION 7

struct PlayfieldAndCabinetSwitch {
   uint8_t switchNum;
   uint8_t solenoid;
   uint8_t solenoidHoldTime;
};

#define SW_SELF_TEST_SWITCH 0x7F
#define SOL_NONE 0x0F
#define SWITCH_STACK_EMPTY 0xFF
#define CONTSOL_DISABLE_FLIPPERS 0x40
#define CONTSOL_DISABLE_COIN_LOCKOUT 0x20

// RPU_InitializeMPU will always boot none of the following
// parameters are set to force it back to original code
#define RPU_CMD_BOOT_ORIGINAL 0x0001 /* This will boot to original unconditionally (disables new code completely for this install) */
#define RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET \
   0x0002 /* Only supported on Rev 4 or greater, boots original if the C/R button is held at power on */
#define RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET \
   0x0004 /* Only supported on Rev 4 or greater, boots original if the C/R button is NOT held at power on */
#define RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED 0x0008     /* boots to original if the switch is closed at power on */
#define RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED 0x0010 /* boots to original if the switch is NOT closed at power on */
#define RPU_CMD_AUTODETECT_ARCHITECTURE \
   0x0040 /* For Rev 101 and greater--the code detects architecture of board (mainly for diagnostics applications) */
#define RPU_CMD_PERFORM_MPU_TEST 0x0080 /* perform basic tests on PIAs and return result codes */

// If the caller chooses this option, it's up to them
// to honor the RPU_RET_ORIGINAL_CODE_REQUESTED return
// flag and halt the Arduino with a while(1);
#define RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN 0x0100

#define RPU_RET_NO_ERRORS 0
#define RPU_RET_U10_PIA_ERROR 0x0001
#define RPU_RET_U11_PIA_ERROR 0x0002
#define RPU_RET_PIA_1_ERROR 0x0004
#define RPU_RET_PIA_2_ERROR 0x0008
#define RPU_RET_PIA_3_ERROR 0x0010
#define RPU_RET_PIA_4_ERROR 0x0020
#define RPU_RET_PIA_5_ERROR 0x0040
#define RPU_RET_OPTION_NOT_SUPPORTED 0x0080
#define RPU_RET_6800_DETECTED 0x0100
#define RPU_RET_6802_OR_8_DETECTED 0x0200
#define RPU_RET_DIAGNOSTIC_REQUESTED 0x1000
#define RPU_RET_SELECTOR_SWITCH_ON 0x2000
#define RPU_RET_CREDIT_RESET_BUTTON_HIT 0x4000
#define RPU_RET_ORIGINAL_CODE_REQUESTED 0x8000

// Function Prototypes

//   Initialization
unsigned long RPU_InitializeMPU(unsigned long initOptions = RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET |
                                                            RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED | RPU_CMD_PERFORM_MPU_TEST,
                                uint8_t creditResetSwitch = 0xFF);
void RPU_SetupGameSwitches(int s_numSwitches, int s_numPrioritySwitches, const PlayfieldAndCabinetSwitch* s_gameSwitchArray);
uint8_t RPU_GetDipSwitches(uint8_t index);

// EEProm Helper Functions
uint8_t RPU_ReadByteFromEEProm(unsigned short startByte);
void RPU_WriteByteToEEProm(unsigned short startByte, uint8_t value);
unsigned long RPU_ReadULFromEEProm(unsigned short startByte, unsigned long defaultValue = 0);
void RPU_WriteULToEEProm(unsigned short startByte, unsigned long value);

//   Swtiches
uint8_t RPU_PullFirstFromSwitchStack();
bool RPU_ReadSingleSwitchState(uint8_t switchNum);
void RPU_PushToSwitchStack(uint8_t switchNumber);
bool RPU_GetUpDownSwitchState(); // This always returns true for RPU_MPU_ARCHITECTURE==1 (no up/down switch)
void RPU_ClearUpDownSwitchState();

//   Solenoids
void RPU_PushToSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes, bool disableOverride = false);
void RPU_SetCoinLockout(bool lockoutOff = false, uint8_t solbit = CONTSOL_DISABLE_COIN_LOCKOUT);
void RPU_SetDisableFlippers(bool disableFlippers = true, uint8_t solbit = CONTSOL_DISABLE_FLIPPERS);
void RPU_SetContinuousSolenoidBit(bool bitOn, uint8_t solBit = 0x10);
#if (RPU_MPU_ARCHITECTURE >= 10)
void RPU_SetContinuousSolenoid(bool solOn, uint8_t solNum);
#endif
bool RPU_FireContinuousSolenoid(uint8_t solBit, uint8_t numCyclesToFire);
uint8_t RPU_ReadContinuousSolenoids();
void RPU_DisableSolenoidStack();
void RPU_EnableSolenoidStack();
bool RPU_PushToTimedSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes, unsigned long whenToFire, bool disableOverride = false);
void RPU_UpdateTimedSolenoidStack(unsigned long curTime);

//   Displays
uint8_t RPU_SetDisplay(int displayNumber, unsigned long value, bool blankByMagnitude = false, uint8_t minDigits = 2,
                       bool showCommasByMagnitude = false);
void RPU_SetDisplayBlank(int displayNumber, uint8_t bitMask);
void RPU_SetDisplayCredits(int value, bool displayOn = true, bool showBothDigits = true);
void RPU_SetDisplayMatch(int value, bool displayOn = true, bool showBothDigits = true);
void RPU_SetDisplayBallInPlay(int value, bool displayOn = true, bool showBothDigits = true);
void RPU_SetDisplayFlash(int displayNumber, unsigned long value, unsigned long curTime, int period = 500, uint8_t minDigits = 2);
void RPU_SetDisplayFlashCredits(unsigned long curTime, int period = 100);
void RPU_CycleAllDisplays(unsigned long curTime, uint8_t digitNum = 0); // Self-test function
uint8_t RPU_GetDisplayBlank(int displayNumber);
#if (RPU_MPU_ARCHITECTURE == 15)
uint8_t RPU_SetDisplayText(int displayNumber, char* text, bool blankByLength = true);
#endif
#if defined(RPU_OS_ADJUSTABLE_DISPLAY_INTERRUPT)
void RPU_SetDisplayRefreshConstant(int intervalConstant);
#endif

//   Lamps
void RPU_SetLampState(int lampNum, uint8_t s_lampState, uint8_t s_lampDim = 0, int s_lampFlashPeriod = 0);
void RPU_ApplyFlashToLamps(unsigned long curTime);
void RPU_FlashAllLamps(unsigned long curTime); // Self-test function
void RPU_TurnOffAllLamps();
void RPU_SetDimDivisor(uint8_t level = 1, uint8_t divisor = 2); // 2 means 50% duty cycle, 3 means 33%, 4 means 25%...
uint8_t RPU_ReadLampState(int lampNum);
uint8_t RPU_ReadLampDim(int lampNum);
int RPU_ReadLampFlash(int lampNum);

// Sound Functions
#ifdef RPU_OS_USE_S_AND_T
void RPU_PlaySoundSAndT(uint8_t soundByte);
#endif

#ifdef RPU_OS_USE_SB100
void RPU_PlaySB100(uint8_t soundByte);
#if (RPU_OS_HARDWARE_REV == 2)
void RPU_PlaySB100Chime(uint8_t soundByte);
#endif
#endif

#ifdef RPU_OS_USE_DASH51
void RPU_PlaySoundDash51(uint8_t soundByte);
#endif

#if (RPU_OS_HARDWARE_REV >= 2 && defined(RPU_OS_USE_SB300))
void RPU_PlaySB300SquareWave(uint8_t soundRegister, uint8_t soundByte);
void RPU_PlaySB300Analog(uint8_t soundRegister, uint8_t soundByte);
#endif

#if defined(RPU_OS_USE_WTYPE_1_SOUND) || defined(RPU_OS_USE_WTYPE_2_SOUND)
void RPU_SetSoundValueLimits(unsigned short lowerLimit, unsigned short upperLimit);
void RPU_PushToSoundStack(unsigned short soundNumber, uint8_t numPushes);
bool RPU_PushToTimedSoundStack(unsigned short soundNumber, uint8_t numPushes, unsigned long whenToPlay);
void RPU_UpdateTimedSoundStack(unsigned long curTime);
#endif
#ifdef RPU_OS_USE_WTYPE_11_SOUND
void RPU_PlayW11Sound(uint8_t soundNum);
void RPU_PlayW11Music(uint8_t songNum);
#endif

//   General
uint8_t RPU_DataRead(int address);
void RPU_Update(unsigned long currentTime);
#if RPU_MPU_ARCHITECTURE > 9
void RPU_SetBoardLEDs(bool LED1, bool LED2, uint8_t BCDValue = 0xFF);
#endif


#define RPU_OS_H
#endif
