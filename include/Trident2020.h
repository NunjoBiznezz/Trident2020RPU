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

#include "Trident.h"
#include "MachineEeprom.h"
#include <stdint.h>


constexpr uint8_t STANDUP_PURPLE_MASK = 0x01;
constexpr uint8_t STANDUP_YELLOW_MASK = 0x02;
constexpr uint8_t STANDUP_AMBER_MASK = 0x04;
constexpr uint8_t STANDUP_GREEN_MASK = 0x08;
constexpr uint8_t STANDUP_WHITE_MASK = 0x10;

constexpr unsigned long SKILL_SHOT_DURATION = 15;
constexpr unsigned long MAX_DISPLAY_BONUS  = 55;
constexpr unsigned long TILT_WARNING_DEBOUNCE_TIME = 1000;
constexpr unsigned long SAUCER_DISPLAY_DURATION = 1000;
constexpr unsigned long MODE_START_DISPLAY_DURATION = 5000;
constexpr unsigned long DROP_TARGET_CLEAR_DURATION = 1000;
constexpr unsigned long STANDUP_HIT_DISPLAY_DURATION = 5000;
constexpr unsigned long MAX_DROP_TARGET_CLEAR_DEADLINE = 5000;
constexpr unsigned long ROLLOVER_FLASH_DURATION = 2000;
constexpr unsigned long RESCUE_FROM_THE_DEEP_TIME = 6000;
constexpr unsigned long FEEDING_FRENZY_ALTERNATE_TIME = 30000;
constexpr unsigned long MODE_QUALIFY_TIME = 45000;
constexpr unsigned long SHARP_SHOOTER_TARGET_TIME = 5000;
constexpr unsigned long MINI_GAME_SINGLE_DURATION = 40000;
constexpr unsigned long MINI_GAME_DOUBLE_DURATION = 66000;
constexpr unsigned long MINI_GAME_TRIPLE_DURATION = 107000;
constexpr unsigned long WIZARD_MODE_DURATION = 110000;

// constexpr uint8_t BONUS_UNDERLIGHTS_OFF = 0;
// constexpr uint8_t BONUS_UNDERLIGHTS_DIM = 1;
// constexpr uint8_t BONUS_UNDERLIGHTS_FULL = 2;



// Game modes
constexpr uint8_t GAME_MODE_SKILL_SHOT = 0;
constexpr uint8_t GAME_MODE_UNSTRUCTURED_PLAY = 1;
constexpr uint8_t GAME_MODE_MINI_GAME_QUALIFIED = 2;
constexpr uint8_t GAME_MODE_MINI_GAME_ENGAGED = 3;
constexpr uint8_t GAME_MODE_MINI_GAME_REWARD_COUNTDOWN = 4;
constexpr uint8_t GAME_MODE_WIZARD_INTRO = 5;
constexpr uint8_t GAME_MODE_FEEDING_FRENZY_FLAG = 0x10;
constexpr uint8_t GAME_MODE_SHARP_SHOOTER_FLAG = 0x20;
constexpr uint8_t GAME_MODE_EXPLORE_THE_DEPTHS_FLAG = 0x40;
constexpr uint8_t GAME_MODE_WIZARD_WITHOUT_FLAGS = 0x0F;
constexpr uint8_t GAME_MODE_WIZARD = 0x7F;

// T2020-specific operator adjustments (103, 105–106, 114–116)
constexpr int EEPROM_SKILL_SHOT_BYTE                 = 103;
constexpr int EEPROM_AWARD_OVERRIDE_BYTE             = 105;
constexpr int EEPROM_BALLS_OVERRIDE_BYTE             = 106;
constexpr int EEPROM_SHARP_SHOOTER_START_BONUS_BYTE  = 114;
constexpr int EEPROM_TARGET_SPECIAL_BONUS_BYTE       = 115;
constexpr int EEPROM_STANDUP_SPECIAL_LEVEL_BYTE      = 116;

// Original Trident high score (117–120)
constexpr int EEPROM_ORIGINAL_HIGHSCORE_BYTE         = 117;  // uint32_t (4 bytes)

// Trident 2020 ruleset scores and audits (140–188)
constexpr int EEPROM_EXTRA_BALL_SCORE_BYTE  = 140;  // uint32_t (4 bytes)
constexpr int EEPROM_SPECIAL_SCORE_BYTE     = 144;  // uint32_t (4 bytes)
constexpr int EEPROM_AWARD_SCORE_1_BYTE     = 148;  // uint32_t (4 bytes)
constexpr int EEPROM_AWARD_SCORE_2_BYTE     = 152;  // uint32_t (4 bytes)
constexpr int EEPROM_AWARD_SCORE_3_BYTE     = 156;  // uint32_t (4 bytes)
constexpr int EEPROM_HIGHSCORE_BYTE         = 160;  // uint32_t (4 bytes)
// 164–185: machine audits — defined in MachineEeprom.h
constexpr int EEPROM_HISCORE_BEAT_BYTE      = 173;  // uint32_t (4 bytes) — T2020 ruleset

// Original Trident ruleset settings (189–221)
constexpr int EEPROM_ORIGINAL_AWARD_OVERRIDE_BYTE    = 189;  // uint8_t  (1 byte)
constexpr int EEPROM_ORIGINAL_AWARD_SCORE_1_BYTE     = 190;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_AWARD_SCORE_2_BYTE     = 194;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_AWARD_SCORE_3_BYTE     = 198;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_EXTRA_BALL_SCORE_BYTE  = 202;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_SPECIAL_SCORE_BYTE     = 206;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_HISCORE_BEAT_BYTE      = 210;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_BALLS_OVERRIDE_BYTE    = 214;  // uint8_t  (1 byte)
// 215–217: freed — now in MachineEeprom.h at addresses 121–123
constexpr int EEPROM_ORIGINAL_DROP_TARGET_SPECIAL_BYTE = 218;  // uint8_t  (1 byte)
constexpr int EEPROM_ORIGINAL_HIGH_SCORE_FEATURE_BYTE  = 219;  // uint8_t  (0=extra ball, 1=replay)
constexpr int EEPROM_ORIGINAL_MELODY_OPTION_BYTE       = 220;  // uint8_t  (1 byte)
constexpr int EEPROM_ORIGINAL_HSTD_FEATURE_BYTE        = 221;  // uint8_t  (1 byte)

constexpr uint8_t SOUND_SELECTOR_NONE = 0;
constexpr uint8_t SOUND_SELECTOR_ORIGINAL = 1;
constexpr uint8_t SOUND_SELECTOR_TRIDENT2020 = 3;

// How many standup clears before Explore the Depths qualifies
constexpr uint8_t ExploreTheDepthsStart = 1;

