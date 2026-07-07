//
// Created by Douglas Bercot on 2/8/26.
// See: https://www.pinballrefresh.com/animation-generator
//

#pragma once

#include <stdint.h>
// #include <api/Common.h>

#define LAMP_BONUS_1                  0	// Q14, J1-18
#define LAMP_BONUS_2                  1	// Q29, J1-1
#define LAMP_BONUS_3                  2	// Q36, J3-26
#define LAMP_BONUS_4                  3	// Q57, J3-1
#define LAMP_BONUS_5                  4	// Q12, J1-19
#define LAMP_BONUS_6                  5	// Q27, J1-9
#define LAMP_BONUS_7                  6	// Q38, J3-25
#define LAMP_BONUS_8                  7	// Q50, J3-12
#define LAMP_BONUS_9                  8	// Q13, J1-17
#define LAMP_BONUS_10                 9	// Q28, J1-8
#define LAMP_DROP_TARGET_5            10	// Q44, J3-19
#define LAMP_STAND_UP_SPECIAL         11	// Q51, J3-15
#define LAMP_LEFT_SPINNER_AMBER       12	// Q8, J1-23
#define LAMP_LEFT_SPINNER_WHITE       13	// Q35, J1-3
#define LAMP_STAND_UP_AMBER           14	// Q49, J3-17
#define LAMP_STAND_UP_WHITE           15	// Q54, J3-11
#define LAMP_RIGHT_SPINNER_YELLOW     16	// Q9, J1-14
#define LAMP_RIGHT_SPINNER_GREEN      17	// Q34, J1-2
#define LAMP_STAND_UP_YELLOW          18	// Q48, J3-16
#define LAMP_STAND_UP_GREEN           19	// Q55, J3-9
#define LAMP_DROP_TARGET_4            20	// Q10, J1-15
#define LAMP_DROP_TARGET_3            21	// Q22, J1-10
#define LAMP_DROP_TARGET_2            22	// Q37, J3-23
#define LAMP_DROP_TARGET_1            23	// Q60, J3-3
#define LAMP_TOP_EJECT_30K            24	// Q11, J1-16
#define LAMP_TOP_EJECT_20K            25	// Q26, J1-7
#define LAMP_TOP_EJECT_10K            26	// Q32, J3-27
#define LAMP_TOP_EJECT_5K             27	// Q59, J3-4
#define LAMP_BONUS_5X_FEATURE         28	// Q4, J1-28
#define LAMP_BONUS_4X_FEATURE         29	// Q25, J1-6
#define LAMP_BONUS_3X_FEATURE         30	// Q20, J1-13
#define LAMP_BONUS_2X_FEATURE         31	// Q58, J3-2
#define LAMP_BONUS_5X                 32	// Q1, J1-24
#define LAMP_BONUS_4X                 33	// Q24, J1-5
#define LAMP_BONUS_3X                 34	// Q42, J3-21
#define LAMP_BONUS_2X                 35	// Q56, J3-10
#define LAMP_LEFT_LANE_8K             36	// Q2, J1-25
#define LAMP_LEFT_LANE_6K             37	// Q17, J1-11
#define LAMP_LEFT_LANE_4K             38	// Q41, J3-20
#define LAMP_LEFT_LANE_2K             39	// Q46, J3-18
#define LAMP_SHOOT_AGAIN              40	// Q3, J2-21
#define LAMP_MATCH                    41	// Q23, J2-8
#define LAMP_STAND_UP_PURPLE          42	// Q40, J2-9
#define LAMP_DROP_TARGET_SPECIAL      43	// Q52, J2-5
#define LAMP_RIGHT_SPINNER_PURPLE     44	// Q7, J2-13
#define LAMP_RIGHT_OUTLANE_SPECIAL    45	// Q21, J2-12
#define LAMP_LEFT_SPINNER_PURPLE      46	// Q39, J2-4
#define LAMP_EXTRA_BALL               47	// Q53, J2-3
#define LAMP_BALL_IN_PLAY             48	// Q16, J2-22
#define LAMP_HIGH_SCORE_TO_DATE       49	// Q15, J2-23
#define LAMP_GAME_OVER                50	// Q33, J2-11
#define LAMP_TILT                     51	// Q47, J2-10
#define LAMP_PLAYER_1                 52	// Q5, J2-16
#define LAMP_PLAYER_2                 53	// Q18, J2-20
#define LAMP_PLAYER_3                 54	// Q30, J2-6
#define LAMP_PLAYER_4                 55	// Q43, J2-7
#define LAMP_PLAYER_1_UP              56	// Q6, J2-14
#define LAMP_PLAYER_2_UP              57	// Q19, J2-15
#define LAMP_PLAYER_3_UP              58	// Q31, J2-2
#define LAMP_PLAYER_4_UP              59	// Q45, J2-1

constexpr unsigned NUM_LAMP_ANIMATIONS = 4;
constexpr unsigned LAMP_ANIMATION_STEPS = 24;
constexpr unsigned NUM_LAMP_ANIMATION_BYTES = 8;

enum class AnimationType : uint8_t {
   RADAR_SWEEP = 0,
   CENTER_OUT = 1,
   BOTTOM_TO_TOP = 2,
   SIDE_TO_SIDE = 3,
};

extern const uint8_t LampAnimations[NUM_LAMP_ANIMATIONS][LAMP_ANIMATION_STEPS][NUM_LAMP_ANIMATION_BYTES];

const uint8_t* PeekAnimationBytes(unsigned animationNum, unsigned frameNum);
bool FetchAnimationBytes(uint8_t *destinationArray, uint8_t animationNum, uint8_t frameNum);

