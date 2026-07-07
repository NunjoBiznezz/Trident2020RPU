#ifndef MACHINE_STATE_H
#define MACHINE_STATE_H

#include <stdint.h>

// Game-level machine states. Mode-internal substates (hardware tests,
// EEPROM settings, adjustments) are owned by their respective mode classes.
constexpr int8_t MACHINE_STATE_ATTRACT          =   0;
constexpr int8_t MACHINE_STATE_INIT_GAMEPLAY    =   1;
constexpr int8_t MACHINE_STATE_INIT_NEW_BALL    =   2;
constexpr int8_t MACHINE_STATE_NORMAL_GAMEPLAY  =   4;
constexpr int8_t MACHINE_STATE_COUNTDOWN_BONUS  =  99;
constexpr int8_t MACHINE_STATE_BALL_OVER        = 100;
constexpr int8_t MACHINE_STATE_MATCH_MODE       = 110;

#endif // MACHINE_STATE_H
