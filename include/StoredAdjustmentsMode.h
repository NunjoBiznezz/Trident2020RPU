/**************************************************************************
 * StoredAdjustmentsMode.h
 *
 * Machine EEPROM settings mode: score levels, high score, credits, audit
 * counters, and coin-per-credit selectors.  All data is stored in the
 * Arduino's own EEPROM (via EEPROM.h) at addresses 148–191, independent
 * of the RPU library's reserved address space (0–52).
 *
 * Setting numbering is local to this mode (1 = score level 1 … 12 = boot).
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "PinballMachine.h"
#include <stdint.h>

class StoredAdjustmentsMode : public MachineMode {
   // --- Setting indices (local to this mode) ---
   static constexpr uint8_t kSaScoreLevel1  =  1;
   static constexpr uint8_t kSaScoreLevel2  =  2;
   static constexpr uint8_t kSaScoreLevel3  =  3;
   static constexpr uint8_t kSaHighScore    =  4;
   static constexpr uint8_t kSaCredits      =  5;
   static constexpr uint8_t kSaTotalPlays   =  6;
   static constexpr uint8_t kSaTotalReplays =  7;
   static constexpr uint8_t kSaHiscrBeat    =  8;
   static constexpr uint8_t kSaChute2Coins  =  9;
   static constexpr uint8_t kSaChute1Coins  = 10;
   static constexpr uint8_t kSaChute3Coins  = 11;
   static constexpr uint8_t kSaBoot         = 12;

   // --- Arduino EEPROM addresses (148–191, after game-settings block ending at 147) ---
   static constexpr int kEeScoreLevel1  = 148;   // unsigned long (4 bytes)
   static constexpr int kEeScoreLevel2  = 152;   // unsigned long (4 bytes)
   static constexpr int kEeScoreLevel3  = 156;   // unsigned long (4 bytes)
   static constexpr int kEeHighScore    = 160;   // unsigned long (4 bytes)
   static constexpr int kEeCredits      = 164;   // uint8_t       (1 byte)
   static constexpr int kEeTotalPlays   = 165;   // unsigned long (4 bytes)
   static constexpr int kEeTotalReplays = 169;   // unsigned long (4 bytes)
   static constexpr int kEeHiscrBeat    = 173;   // unsigned long (4 bytes)
   static constexpr int kEeChute2Coins  = 177;   // unsigned long (4 bytes)
   static constexpr int kEeChute1Coins  = 181;   // unsigned long (4 bytes)
   static constexpr int kEeChute3Coins  = 185;   // unsigned long (4 bytes)
   // CPC selections managed through machine_ interface (RPU EEPROM), indices 13–15.

   uint8_t internalState_ = 0;
   bool    stateChanged_  = false;

   PinballMachine* machine_ = nullptr;

   unsigned long savedValue_            = 0;
   unsigned long resetHold_             = 0;
   unsigned long nextSpeedyValueChange_ = 0;
   unsigned long numSpeedyChanges_      = 0;
   unsigned long lastResetPress_        = 0;

public:
   void setDependencies(PinballMachine& machine) {
      machine_ = &machine;
   }

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
