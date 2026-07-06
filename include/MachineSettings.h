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
   uint8_t       credits                  = 0;
   uint8_t       maximumCredits           = 40;
   uint8_t       ballsPerGame             = 3;
   uint8_t       ballSaveNumSeconds       = 0;
   bool          freePlayMode             = true;
   bool          tournamentScoring        = false;
   bool          scrollingScores          = true;
   uint8_t       scoreAwardReplay         = 0;
   uint8_t       maxTiltWarnings          = 2;
   bool          matchFeature             = true;
   bool          highScoreReplay          = true;

   // --- Scores / awards ---
   unsigned long highScore                = 0;
   unsigned long awardScores[3]           = {};
   unsigned long extraBallValue           = 0;
   unsigned long specialValue             = 0;

   // --- Game balance ---
   uint8_t       sharpShooterStartBonus   = 3;
   uint8_t       targetSpecialBonus       = 4;
   uint8_t       standupSpecialLevel      = 2;

   // --- Display ---
   uint8_t       dimLevel                 = 2;

   // --- Audio ---
   uint8_t       soundSelector            = 3;
   uint8_t       musicVolume              = 10;
   uint8_t       sfxVolume                = 10;
   uint8_t       calloutsVolume           = 10;

   // --- Transient flags (not EEPROM-backed) ---
   bool          resetScoresToClearVersion = false;
};
