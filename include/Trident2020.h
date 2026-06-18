/**************************************************************************
 *     This file is part of Trident2020.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 6/1/2020

    Trident2020 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Trident2020 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>

// Lamp Numbers (defines for lamps)
typedef uint8_t LampNumber_t;
constexpr LampNumber_t BONUS_1 = 0;
constexpr LampNumber_t BONUS_2 = 1;
constexpr LampNumber_t BONUS_3 = 2;
constexpr LampNumber_t BONUS_4 = 3;
constexpr LampNumber_t BONUS_5 = 4;
constexpr LampNumber_t BONUS_6 = 5;
constexpr LampNumber_t BONUS_7 = 6;
constexpr LampNumber_t BONUS_8 = 7;
constexpr LampNumber_t BONUS_9 = 8;
constexpr LampNumber_t BONUS_10 = 9;
constexpr LampNumber_t DROP_TARGET_5 = 10;
constexpr LampNumber_t STAND_UP_SPECIAL = 11;
constexpr LampNumber_t LEFT_SPINNER_AMBER = 12;
constexpr LampNumber_t LEFT_SPINNER_WHITE = 13;
constexpr LampNumber_t STAND_UP_AMBER = 14;
constexpr LampNumber_t STAND_UP_WHITE = 15;
constexpr LampNumber_t RIGHT_SPINNER_YELLOW = 16;
constexpr LampNumber_t RIGHT_SPINNER_GREEN = 17;
constexpr LampNumber_t STAND_UP_YELLOW = 18;
constexpr LampNumber_t STAND_UP_GREEN = 19;
constexpr LampNumber_t DROP_TARGET_4 = 20;
constexpr LampNumber_t DROP_TARGET_3 = 21;
constexpr LampNumber_t DROP_TARGET_2 = 22;
constexpr LampNumber_t DROP_TARGET_1 = 23;
constexpr LampNumber_t TOP_EJECT_30K = 24;
constexpr LampNumber_t TOP_EJECT_20K = 25;
constexpr LampNumber_t TOP_EJECT_10K = 26;
constexpr LampNumber_t TOP_EJECT_5K = 27;
constexpr LampNumber_t BONUS_5X_FEATURE = 28;
constexpr LampNumber_t BONUS_4X_FEATURE = 29;
constexpr LampNumber_t BONUS_3X_FEATURE = 30;
constexpr LampNumber_t BONUS_2X_FEATURE = 31;
constexpr LampNumber_t BONUS_5X = 32;
constexpr LampNumber_t BONUS_4X = 33;
constexpr LampNumber_t BONUS_3X = 34;
constexpr LampNumber_t BONUS_2X = 35;
constexpr LampNumber_t LEFT_LANE_8K = 36;
constexpr LampNumber_t LEFT_LANE_6K = 37;
constexpr LampNumber_t LEFT_LANE_4K = 38;
constexpr LampNumber_t LEFT_LANE_2K = 39;
constexpr LampNumber_t SHOOT_AGAIN = 40;
constexpr LampNumber_t MATCH = 41;
constexpr LampNumber_t STAND_UP_PURPLE = 42;
constexpr LampNumber_t DROP_TARGET_SPECIAL = 43;
constexpr LampNumber_t RIGHT_SPINNER_PURPLE = 44;
constexpr LampNumber_t RIGHT_OUTLANE_SPECIAL = 45;
constexpr LampNumber_t LEFT_SPINNER_PURPLE = 46;
constexpr LampNumber_t EXTRA_BALL = 47;
constexpr LampNumber_t BALL_IN_PLAY = 48;
constexpr LampNumber_t HIGH_SCORE_TO_DATE = 49;
constexpr LampNumber_t GAME_OVER = 50;
constexpr LampNumber_t TILT = 51;
constexpr LampNumber_t PLAYER_1 = 52;
constexpr LampNumber_t PLAYER_2 = 53;
constexpr LampNumber_t PLAYER_3 = 54;
constexpr LampNumber_t PLAYER_4 = 55;
constexpr LampNumber_t PLAYER_1_UP = 56;
constexpr LampNumber_t PLAYER_2_UP = 57;
constexpr LampNumber_t PLAYER_3_UP = 58;
constexpr LampNumber_t PLAYER_4_UP = 59;

// Defines for switches
constexpr uint8_t SW_CREDIT_RESET = 5;
constexpr uint8_t SW_TILT = 6;
constexpr uint8_t SW_OUTHOLE = 32;
constexpr uint8_t SW_COIN_1 = 1;
constexpr uint8_t SW_COIN_2 = 0;
constexpr uint8_t SW_COIN_3 = 2;
constexpr uint8_t SW_SLAM = 7;

constexpr uint8_t SW_DROP_TARGET_5 = 27;
constexpr uint8_t SW_DROP_TARGET_4 = 28;
constexpr uint8_t SW_DROP_TARGET_3 = 29;
constexpr uint8_t SW_DROP_TARGET_2 = 30;
constexpr uint8_t SW_DROP_TARGET_1 = 31;

constexpr uint8_t SW_SAUCER = 25;
constexpr uint8_t SW_RIGHT_INLANE = 17;
constexpr uint8_t SW_LEFT_INLANE = 18;
constexpr uint8_t SW_10_PTS = 26;
constexpr uint8_t SW_RIGHT_OUTLANE = 16;
constexpr uint8_t SW_TOP_BUMPER = 14;
constexpr uint8_t SW_BOTTOM_BUMPER = 15;

constexpr uint8_t SW_WHITE = 23;
constexpr uint8_t SW_GREEN = 22;
constexpr uint8_t SW_AMBER = 21;
constexpr uint8_t SW_YELLOW = 20;
constexpr uint8_t SW_PURPLE = 19;

constexpr uint8_t SW_LEFT_SPINNER = 4;
constexpr uint8_t SW_RIGHT_SPINNER = 3;

constexpr uint8_t SW_UL_SLING = 11;
constexpr uint8_t SW_UR_SLING = 10;
constexpr uint8_t SW_LL_SLING = 13;
constexpr uint8_t SW_LR_SLING = 12;

constexpr uint8_t SW_ROLLOVER = 9;

// Defines for solenoids
constexpr uint8_t SOL_TOP_BUMPER = 0;
constexpr uint8_t SOL_BOTTOM_BUMPER = 1;
constexpr uint8_t SOL_UL_SLING = 2;
constexpr uint8_t SOL_DROP_TARGET_1 = 3;
constexpr uint8_t SOL_DROP_TARGET_2 = 4;
constexpr uint8_t SOL_KNOCKER = 5;
constexpr uint8_t SOL_LL_SLING = 6;
constexpr uint8_t SOL_DROP_TARGET_3 = 7;
constexpr uint8_t SOL_UR_SLING = 8;
constexpr uint8_t SOL_LR_SLING = 9;
constexpr uint8_t SOL_DROP_TARGET_4 = 10;
constexpr uint8_t SOL_DROP_TARGET_5 = 11;
constexpr uint8_t SOL_SAUCER = 12;
constexpr uint8_t SOL_DROP_TARGET_RESET = 13;
constexpr uint8_t SOL_OUTHOLE = 14;

// SWITCHES_WITH_TRIGGERS are for switches that will automatically
// activate a solenoid (like in the case of a chime that rings on a rollover)
// but SWITCHES_WITH_TRIGGERS are fully debounced before being activated
constexpr unsigned NUM_SWITCHES_WITH_TRIGGERS = 6;

// PRIORITY_SWITCHES_WITH_TRIGGERS are switches that trigger immediately
// (like for pop bumpers or slings) - they are not debounced completely
constexpr unsigned NUM_PRIORITY_SWITCHES_WITH_TRIGGERS = 6;

// Define automatic solenoid triggers (switch, solenoid, number of 1/120ths of a second to fire)
const struct PlayfieldAndCabinetSwitch TriggeredSwitches[NUM_SWITCHES_WITH_TRIGGERS] = {
    {SW_TOP_BUMPER,    SOL_TOP_BUMPER,    4},
    {SW_BOTTOM_BUMPER, SOL_BOTTOM_BUMPER, 4},
    {SW_UL_SLING,      SOL_UL_SLING,      4},
    {SW_LL_SLING,      SOL_LL_SLING,      4},
    {SW_UR_SLING,      SOL_UR_SLING,      4},
    {SW_LR_SLING,      SOL_LR_SLING,      4},
};
