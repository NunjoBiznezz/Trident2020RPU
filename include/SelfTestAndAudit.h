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


#ifndef SELF_TEST_H
#define SELF_TEST_H

#include <stdint.h>

constexpr int MACHINE_STATE_TEST_LAMPS        =  -1;
constexpr int MACHINE_STATE_TEST_DISPLAYS       =  -2;
constexpr int MACHINE_STATE_TEST_SOLENOIDS      =  -3;
constexpr int MACHINE_STATE_TEST_SWITCHES       =  -4;
constexpr int MACHINE_STATE_TEST_SOUNDS         =  -5;
constexpr int MACHINE_STATE_TEST_SCORE_LEVEL_1  =  -6;
constexpr int MACHINE_STATE_TEST_SCORE_LEVEL_2  =  -7;
constexpr int MACHINE_STATE_TEST_SCORE_LEVEL_3  =  -8;
constexpr int MACHINE_STATE_TEST_HISCR          =  -9;
constexpr int MACHINE_STATE_TEST_CREDITS        =  -10;
constexpr int MACHINE_STATE_TEST_TOTAL_PLAYS    =  -11;
constexpr int MACHINE_STATE_TEST_TOTAL_REPLAYS  =  -12;
constexpr int MACHINE_STATE_TEST_HISCR_BEAT     =  -13;
constexpr int MACHINE_STATE_TEST_CHUTE_2_COINS  =  -14;
constexpr int MACHINE_STATE_TEST_CHUTE_1_COINS  =  -15;
constexpr int MACHINE_STATE_TEST_CHUTE_3_COINS  =  -16;
constexpr int MACHINE_STATE_TEST_BOOT           =  -17;

constexpr int MACHINE_STATE_ADJUST_CPC_CHUTE_1  =   -18;
constexpr int MACHINE_STATE_ADJUST_CPC_CHUTE_2  =   -19;
constexpr int MACHINE_STATE_ADJUST_CPC_CHUTE_3  =   -20;

// This define is set to the last test, so the extended settings will know when to take over
constexpr int MACHINE_STATE_TEST_DONE           =  -20;

unsigned long GetLastSelfTestChangedTime();
void SetLastSelfTestChangedTime(unsigned long setSelfTestChange);
int RunBaseSelfTest(int curState, bool curStateChanged, unsigned long CurrentTime, uint8_t resetSwitch, uint8_t slamSwitch=0xFF);

unsigned long GetAwardScore(uint8_t level);
#ifndef RPU_OS_DISABLE_CPC_FOR_SPACE
uint8_t GetCPCSelection(uint8_t chuteNumber);
uint8_t GetCPCCoins(uint8_t cpcSelection);
uint8_t GetCPCCredits(uint8_t cpcSelection);
#endif

#endif
