/**************************************************************************
 * MachineSettings.h
 *
 * All operator-adjustable and EEPROM-backed machine state in one place.
 * TridentMachine owns the single instance; other objects hold a pointer
 * or reference to it.
 **************************************************************************/

#pragma once
#include <stdint.h>

struct MachineSettings {
   // --- Credits / play options ---
   uint8_t       credits                  = 0;     // Current stored credits (loaded from RPU EEPROM at boot)
   uint8_t       maximumCredits           = 99;    // Hard cap on stored credits
   uint8_t       ballsPerGame             = 3;     // 3 or 5; EEPROM_BALLS_OVERRIDE_BYTE must be 3 or 5 to change
   uint8_t       ballSaveNumSeconds       = 0;     // Ball-save grace period after launch (0 = off, max 20 s)
   bool          freePlayMode             = true;  // When true, START never requires credits
   bool          tournamentScoring        = false; // When true, score-based awards (EB, special) are suppressed
   bool          scrollingScores          = true;  // When true, scores above 999 999 scroll across the display
   uint8_t       scoreAwardReplay         = 0;     // Bitmask: bit N set → awardScores[N] gives a replay; clear → extra ball
   uint8_t       maxTiltWarnings          = 2;     // Tilt warnings before ball loss (0–2)
   bool          matchFeature             = true;  // When true, a match on the last two score digits gives a free credit
   bool          highScoreReplay          = true;  // When true, beating the high score awards a replay

   // --- Scores / awards ---
   unsigned long highScore                = 0;     // All-time high score (RPU EEPROM)
   unsigned long awardScores[3]           = {};    // Three score thresholds for replay / extra-ball awards
   unsigned long extraBallValue           = 0;     // Points awarded for an extra ball in tournament mode
   unsigned long specialValue             = 0;     // Points awarded for a special in tournament mode

   // --- Game balance ---
   uint8_t       sharpShooterStartBonus   = 3;    // Bonus multiplier at which Sharp Shooter mini-game qualifies
   uint8_t       targetSpecialBonus       = 4;    // Bonus multiplier that lights the drop-target special
   uint8_t       standupSpecialLevel      = 2;    // Standup-bank clear count that lights the right-outlane special

   // --- Display ---
   uint8_t       dimLevel                 = 2;    // RPU lamp dim divisor (2 or 3); higher = dimmer

   // --- Audio ---
   uint8_t       soundSelector            = 3;    // 0=none, 1=original hardware card, 3=WAV Trigger (Trident2020)
   uint8_t       musicVolume              = 10;   // Background music volume 0–10
   uint8_t       sfxVolume                = 10;   // Sound-effects volume 0–10
   uint8_t       calloutsVolume           = 10;   // Voice callout volume 0–10

   // --- Transient flags (not EEPROM-backed) ---
   bool          resetScoresToClearVersion = false; // Game sets this to request a score-display clear on version change
};

constexpr uint8_t TILT_WARNINGS_DEFAULT = 2;
constexpr uint8_t TILT_WARNINGS_MAX = 2;

constexpr uint8_t AWARD_OVERRIDE_DEFAULT = 99;
constexpr uint8_t AWARD_OVERRIDE_MAX = 99;

constexpr uint8_t BALLS_PER_GAME_DEFAULT = 5;

constexpr uint8_t BALL_SAVE_TIME_S_DEFAULT = 15;
constexpr uint8_t BALL_SAVE_TIME_S_MAX = 20;
