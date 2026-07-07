//
// Created by Douglas Bercot on 2/8/26.
// See: https://www.pinballrefresh.com/animation-generator
//

#pragma once

#include <stdint.h>

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

