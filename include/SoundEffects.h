#ifndef SOUND_EFFECTS_H
#define SOUND_EFFECTS_H

#include <stdint.h>

/*********************************************************************
    Native SB100 Effect Definitions
*********************************************************************/
constexpr uint8_t SOUND_NATIVE_NONE = 0x00;           // This turns sounds off
constexpr uint8_t SOUND_NATIVE_TEN = 0x01;            // This plays the ten sound
constexpr uint8_t SOUND_NATIVE_ONE_HUNDRED = 0x02;    // This plays the one hundred sound
constexpr uint8_t SOUND_NATIVE_ONE_THOUSAND = 0x04;   // This plays the one thousand sound
constexpr uint8_t SOUND_NATIVE_TEN_THOUSAND = 0x08;   // This plays the ten thousand sound
constexpr uint8_t SOUND_NATIVE_ADD_BONUS = 0x10;      // This plays the add bonus sound
constexpr uint8_t SOUND_NATIVE_POP_BUMPER = 0x20;     // This plays the pop bumper sound

/*********************************************************************
    SB100 Sampled sounds for the WAV player
*********************************************************************/
constexpr uint8_t SOUND_EFFECT_TEN = 100;             // This plays the ten sound
constexpr uint8_t SOUND_EFFECT_ONE_HUNDRED = 101;     // This plays the one hundred sound
constexpr uint8_t SOUND_EFFECT_ONE_THOUSAND = 102;    // This plays the one thousand sound
constexpr uint8_t SOUND_EFFECT_TEN_THOUSAND = 103;    // This plays the ten thousand sound
constexpr uint8_t SOUND_EFFECT_ADD_BONUS = 104;       // This plays the add bonus sound
constexpr uint8_t SOUND_EFFECT_POP_BUMPER = 105;      // This plays the pop bumper sound
constexpr uint8_t SOUND_EFFECT_INTRO = 106;           // This plays the intro sound

/*********************************************************************
    Sound Tracks
*********************************************************************/
constexpr uint8_t SOUND_EFFECT_NONE = 0;
constexpr uint8_t SOUND_EFFECT_DT_SKILL_SHOT = 1;
constexpr uint8_t SOUND_EFFECT_ROLLOVER_SKILL_SHOT = 2;
constexpr uint8_t SOUND_EFFECT_SU_SKILL_SHOT = 3;
constexpr uint8_t SOUND_EFFECT_LEFT_SPINNER = 4;
constexpr uint8_t SOUND_EFFECT_RIGHT_SPINNER = 5;
constexpr uint8_t SOUND_EFFECT_ROLLOVER = 6;
constexpr uint8_t SOUND_EFFECT_LEFT_INLANE = 7;
constexpr uint8_t SOUND_EFFECT_RIGHT_INLANE = 8;
constexpr uint8_t SOUND_EFFECT_RIGHT_OUTLANE = 9;
constexpr uint8_t SOUND_EFFECT_TOP_BUMPER_HIT = 10;
constexpr uint8_t SOUND_EFFECT_BOTTOM_BUMPER_HIT = 11;
constexpr uint8_t SOUND_EFFECT_DROP_TARGET = 12;
constexpr uint8_t SOUND_EFFECT_ADD_CREDIT = 13;
constexpr uint8_t SOUND_EFFECT_RESCUE_FROM_THE_DEEP = 14;
constexpr uint8_t SOUND_EFFECT_SHOOT_AGAIN = 15;
constexpr uint8_t SOUND_EFFECT_FEEDING_FRENZY = 16;
constexpr uint8_t SOUND_EFFECT_FEEDING_FRENZY_START = 17;
constexpr uint8_t SOUND_EFFECT_SHARP_SHOOTER_START = 18;
constexpr uint8_t SOUND_EFFECT_JACKPOT = 19;
constexpr uint8_t SOUND_EFFECT_ADD_PLAYER_1 = 20;
constexpr uint8_t SOUND_EFFECT_ADD_PLAYER_2 = SOUND_EFFECT_ADD_PLAYER_1 + 1;
constexpr uint8_t SOUND_EFFECT_ADD_PLAYER_3 = SOUND_EFFECT_ADD_PLAYER_1 + 2;
constexpr uint8_t SOUND_EFFECT_ADD_PLAYER_4 = SOUND_EFFECT_ADD_PLAYER_1 + 3;
constexpr uint8_t SOUND_EFFECT_PLAYER_1_UP = 24;
constexpr uint8_t SOUND_EFFECT_PLAYER_2_UP = SOUND_EFFECT_PLAYER_1_UP + 1;
constexpr uint8_t SOUND_EFFECT_PLAYER_3_UP = SOUND_EFFECT_PLAYER_1_UP + 2;
constexpr uint8_t SOUND_EFFECT_PLAYER_4_UP = SOUND_EFFECT_PLAYER_1_UP + 3;
constexpr uint8_t SOUND_EFFECT_BALL_OVER = 30;
constexpr uint8_t SOUND_EFFECT_GAME_OVER = 31;
constexpr uint8_t SOUND_EFFECT_BONUS_COUNT = 32;
constexpr uint8_t SOUND_EFFECT_2X_BONUS_COUNT = 33;
constexpr uint8_t SOUND_EFFECT_3X_BONUS_COUNT = 34;
constexpr uint8_t SOUND_EFFECT_4X_BONUS_COUNT = 35;
constexpr uint8_t SOUND_EFFECT_5X_BONUS_COUNT = 36;
constexpr uint8_t SOUND_EFFECT_EXTRA_BALL = 37;
constexpr uint8_t SOUND_EFFECT_TILT_WARNING = 39;
constexpr uint8_t SOUND_EFFECT_MATCH_SPIN = 40;
constexpr uint8_t SOUND_EFFECT_LOWER_SLING = 41;
constexpr uint8_t SOUND_EFFECT_UPPER_SLING = 42;
constexpr uint8_t SOUND_EFFECT_10PT_SWITCH = 43;
constexpr uint8_t SOUND_EFFECT_SAUCER_HIT_5K = 44;
constexpr uint8_t SOUND_EFFECT_SAUCER_HIT_10K = 45;
constexpr uint8_t SOUND_EFFECT_SAUCER_HIT_20K = 46;
constexpr uint8_t SOUND_EFFECT_SAUCER_HIT_30K = 47;
constexpr uint8_t SOUND_EFFECT_SHARP_SHOOTER_HIT = 48;
constexpr uint8_t SOUND_EFFECT_DROP_TARGET_CLEAR_1 = 50;
constexpr uint8_t SOUND_EFFECT_DROP_TARGET_CLEAR_2 = 51;
constexpr uint8_t SOUND_EFFECT_DROP_TARGET_CLEAR_3 = 52;
constexpr uint8_t SOUND_EFFECT_DROP_TARGET_CLEAR_4 = 53;
constexpr uint8_t SOUND_EFFECT_DROP_TARGET_CLEAR_5 = 54;
constexpr uint8_t SOUND_EFFECT_FIRST_SU_SWITCH_HIT = 55;
constexpr uint8_t SOUND_EFFECT_SECOND_SU_SWITCH_HIT = 56;
constexpr uint8_t SOUND_EFFECT_THIRD_SU_SWITCH_HIT = 57;
constexpr uint8_t SOUND_EFFECT_FOURTH_SU_SWITCH_HIT = 58;
constexpr uint8_t SOUND_EFFECT_FIFTH_SU_SWITCH_HIT = 59;
constexpr uint8_t SOUND_EFFECT_EXPLORE_QUALIFIED = 60;
constexpr uint8_t SOUND_EFFECT_STANDUPS_CLEARED = 61;
constexpr uint8_t SOUND_EFFECT_EXPLORE_HIT = 62;
constexpr uint8_t SOUND_EFFECT_EXPLORE_THE_DEPTHS_START = 63;
constexpr uint8_t SOUND_EFFECT_SHARP_SHOOTER_QUALIFIED = 65;
constexpr uint8_t SOUND_EFFECT_FEEDING_FRENZY_QUALIFIED = 66;
constexpr uint8_t SOUND_EFFECT_MODE_FINISHED = 67;
constexpr uint8_t SOUND_EFFECT_SS_AND_FF_START = 68;
constexpr uint8_t SOUND_EFFECT_SS_AND_ETD_START = 69;
constexpr uint8_t SOUND_EFFECT_FF_AND_ETD_START = 70;
constexpr uint8_t SOUND_EFFECT_MEGA_STACK_START = 71;
constexpr uint8_t SOUND_EFFECT_SWIM_AGAIN = 72;
constexpr uint8_t SOUND_EFFECT_DEEP_BLUE_SEA_MODE = 73;
constexpr uint8_t SOUND_EFFECT_TRIDENT_INTRO = 89;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_1 = 90;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_2 = 91;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_3 = 92;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_4 = 93;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_5 = 94;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_6 = 95;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_WIZ = 96;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_FOR_SINGLE_MODE = 97;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_FOR_DOUBLE_MODE = 98;
constexpr uint8_t SOUND_EFFECT_BACKGROUND_FOR_TRIPLE_MODE = 99;
constexpr uint8_t SOUND_EFFECT_COIN_DROP_1 = 100;  // TODO these are not on the sound card ATM
constexpr uint8_t SOUND_EFFECT_COIN_DROP_2 = 101;  // TODO these are not on the sound card ATM
constexpr uint8_t SOUND_EFFECT_COIN_DROP_3 = 102;  // TODO these are not on the sound card ATM



#endif // SOUND_EFFECTS_H
