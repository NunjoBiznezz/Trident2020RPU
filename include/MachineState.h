#ifndef MACHINE_STATE_H
#define MACHINE_STATE_H

#include <stdint.h>

constexpr int MACHINE_STATE_TEST_LAMPS          =  -1;
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

constexpr int MACHINE_STATE_ADJUST_CPC_CHUTE_1  =  -18;
constexpr int MACHINE_STATE_ADJUST_CPC_CHUTE_2  =  -19;
constexpr int MACHINE_STATE_ADJUST_CPC_CHUTE_3  =  -20;

// Boundary between hardware tests and operator adjustments.
constexpr int MACHINE_STATE_TEST_DONE           =  -20;

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
constexpr int8_t MACHINE_STATE_ATTRACT = 0;
constexpr int8_t MACHINE_STATE_INIT_GAMEPLAY = 1;
constexpr int8_t MACHINE_STATE_INIT_NEW_BALL = 2;
constexpr int8_t MACHINE_STATE_NORMAL_GAMEPLAY = 4;
constexpr int8_t MACHINE_STATE_COUNTDOWN_BONUS = 99;
constexpr int8_t MACHINE_STATE_BALL_OVER = 100;
constexpr int8_t MACHINE_STATE_MATCH_MODE = 110;

constexpr int8_t MACHINE_STATE_ADJUST_FREEPLAY           = (MACHINE_STATE_TEST_DONE - 1);
constexpr int8_t MACHINE_STATE_ADJUST_BALL_SAVE          = (MACHINE_STATE_TEST_DONE - 2);
constexpr int8_t MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK = (MACHINE_STATE_TEST_DONE - 3);
constexpr int8_t MACHINE_STATE_ADJUST_MUSIC_VOLUME       = (MACHINE_STATE_TEST_DONE - 4);
constexpr int8_t MACHINE_STATE_ADJUST_SFX_VOLUME         = (MACHINE_STATE_TEST_DONE - 5);
constexpr int8_t MACHINE_STATE_ADJUST_CALLOUTS_VOLUME    = (MACHINE_STATE_TEST_DONE - 6);
constexpr int8_t MACHINE_STATE_ADJUST_TOURNAMENT_SCORING = (MACHINE_STATE_TEST_DONE - 7);
constexpr int8_t MACHINE_STATE_ADJUST_TILT_WARNING       = (MACHINE_STATE_TEST_DONE - 8);
constexpr int8_t MACHINE_STATE_ADJUST_AWARD_OVERRIDE     = (MACHINE_STATE_TEST_DONE - 9);
constexpr int8_t MACHINE_STATE_ADJUST_BALLS_OVERRIDE     = (MACHINE_STATE_TEST_DONE - 10);
constexpr int8_t MACHINE_STATE_ADJUST_SCROLLING_SCORES   = (MACHINE_STATE_TEST_DONE - 11);
constexpr int8_t MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD   = (MACHINE_STATE_TEST_DONE - 12);
constexpr int8_t MACHINE_STATE_ADJUST_SPECIAL_AWARD      = (MACHINE_STATE_TEST_DONE - 13);
constexpr int8_t MACHINE_STATE_ADJUST_DIM_LEVEL          = (MACHINE_STATE_TEST_DONE - 14);
constexpr int8_t MACHINE_STATE_ADJUST_DONE               = (MACHINE_STATE_TEST_DONE - 15);

#endif // MACHINE_STATE_H
