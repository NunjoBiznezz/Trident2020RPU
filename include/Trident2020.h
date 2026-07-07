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

constexpr int EEPROM_BALL_SAVE_BYTE = 100;
constexpr int EEPROM_FREE_PLAY_BYTE = 101;
constexpr int EEPROM_SOUND_SELECTOR_BYTE = 102;
constexpr int EEPROM_SKILL_SHOT_BYTE = 103;
constexpr int EEPROM_TILT_WARNING_BYTE = 104;
constexpr int EEPROM_AWARD_OVERRIDE_BYTE = 105;
constexpr int EEPROM_BALLS_OVERRIDE_BYTE = 106;
constexpr int EEPROM_TOURNAMENT_SCORING_BYTE = 107;
constexpr int EEPROM_MUSIC_VOLUME_BYTE = 108;
constexpr int EEPROM_SFX_VOLUME_BYTE = 109;
constexpr int EEPROM_SCROLLING_SCORES_BYTE = 110;
constexpr int EEPROM_CALLOUTS_VOLUME_BYTE = 111;
constexpr int EEPROM_DIM_LEVEL_BYTE = 113;
constexpr int EEPROM_EXTRA_BALL_SCORE_BYTE = 140;
constexpr int EEPROM_SPECIAL_SCORE_BYTE = 144;

constexpr uint8_t SOUND_SELECTOR_NONE = 0;
constexpr uint8_t SOUND_SELECTOR_ORIGINAL = 1;
constexpr uint8_t SOUND_SELECTOR_TRIDENT2020 = 3;

// How many standup clears before Explore the Depths qualifies
constexpr uint8_t ExploreTheDepthsStart = 1;

