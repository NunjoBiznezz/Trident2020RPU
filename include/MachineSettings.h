/**************************************************************************
 * MachineSettings.h
 *
 * All operator-adjustable and EEPROM-backed machine state in one place.
 * PinballMachine owns the single instance; other objects hold a pointer
 * or reference to it.
 **************************************************************************/

#pragma once
#include <stdint.h>

enum class RuleSet : uint8_t {
   Original   = 0,
   Trident2020 = 1,
};

// Original Trident rule-specific settings: scores, awards, and audit.
struct TridentGameSettings {
   unsigned long highScore        = 0;
   unsigned long awardScores[3]   = {};
   unsigned long extraBallValue   = 0;
   unsigned long specialValue     = 0;
   uint8_t       scoreAwardReplay = 0;
   uint32_t      hiscoreBeat      = 0;
};

// Trident 2020 rule-specific settings: scores, awards, audit, and game balance.
struct Trident2020GameSettings {
   unsigned long highScore        = 0;
   unsigned long awardScores[3]   = {};
   unsigned long extraBallValue   = 0;
   unsigned long specialValue     = 0;
   uint8_t       scoreAwardReplay = 0;
   uint32_t      hiscoreBeat      = 0;
   uint8_t sharpShooterStartBonus = 3;
   uint8_t targetSpecialBonus     = 4;
   uint8_t standupSpecialLevel    = 2;
};

struct MachineSettings {
   // --- Credits / play options ---
   uint8_t       credits                  = 0;     // Current stored credits (loaded from RPU EEPROM at boot)
   uint8_t       maximumCredits           = 99;    // Hard cap on stored credits
   uint8_t       ballsPerGame             = 3;     // 3 or 5; EEPROM_BALLS_OVERRIDE_BYTE must be 3 or 5 to change
   uint8_t       ballSaveNumSeconds       = 0;     // Ball-save grace period after launch (0 = off, max 20 s)
   bool          freePlayMode             = true;  // When true, START never requires credits
   bool          tournamentScoring        = false; // When true, score-based awards (EB, special) are suppressed
   bool          scrollingScores          = true;  // When true, scores above 999 999 scroll across the display
   uint8_t       maxTiltWarnings          = 2;     // Tilt warnings before ball loss (0–2)
   bool          matchFeature             = true;  // When true, a match on the last two score digits gives a free credit
   bool          highScoreReplay          = true;  // When true, beating the high score awards a replay

   // --- Rule set selection ---
   RuleSet       activeRuleSet            = RuleSet::Trident2020;

   // --- Per-ruleset: scores, awards, audit, and game balance ---
   TridentGameSettings     tridentSettings;
   Trident2020GameSettings trident2020Settings;

   // --- Display ---
   uint8_t       dimLevel                 = 2;    // RPU lamp dim divisor (2 or 3); higher = dimmer

   // --- Audio ---
   uint8_t       soundSelector            = 3;    // 0=none, 1=original hardware card, 3=WAV Trigger (Trident2020)
   uint8_t       musicVolume              = 10;   // Background music volume 0–10
   uint8_t       sfxVolume                = 10;   // Sound-effects volume 0–10
   uint8_t       calloutsVolume           = 10;   // Voice callout volume 0–10

   // --- Audit counters (incremented by game events; reset via service menu) ---
   uint32_t      totalPlays   = 0;
   uint32_t      totalReplays = 0;

   // --- Coin chute counts ---
   uint32_t      chute1Coins  = 0;
   uint32_t      chute2Coins  = 0;
   uint32_t      chute3Coins  = 0;

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
