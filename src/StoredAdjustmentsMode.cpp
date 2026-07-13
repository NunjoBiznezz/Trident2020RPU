/**************************************************************************
 * StoredAdjustmentsMode.cpp
 **************************************************************************/

#include "StoredAdjustmentsMode.h"
#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include "adjustments/Adjustments.h"

// ---------------------------------------------------------------------------
// EEPROM addresses (148–185, after game-settings block ending at 147)
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Adjustment objects — heterogeneous types require separate named variables;
// the dispatch table below binds each one to its entry callout index.
// ---------------------------------------------------------------------------

static ScoreAdjustment   sScoreLevel1  { kEeScoreLevel1  };
static ScoreAdjustment   sScoreLevel2  { kEeScoreLevel2  };
static ScoreAdjustment   sScoreLevel3  { kEeScoreLevel3  };
static ScoreAdjustment   sHighScore    { kEeHighScore     };
static CreditsAdjustment sCredits      { kEeCredits       };
static AuditAdjustment   sTotalPlays   { kEeTotalPlays    };
static AuditAdjustment   sTotalReplays { kEeTotalReplays  };
static AuditAdjustment   sHiscrBeat    { kEeHiscrBeat     };
static AuditAdjustment   sChute2Coins  { kEeChute2Coins   };
static AuditAdjustment   sChute1Coins  { kEeChute1Coins   };
static AuditAdjustment   sChute3Coins  { kEeChute3Coins   };
static BootAdjustment    sBoot;

struct AdjEntry {
   StoredAdjustment* adj;
   uint8_t           callout;
};

static AdjEntry kAdjustments[] = {
   { &sScoreLevel1,  140 },
   { &sScoreLevel2,  141 },
   { &sScoreLevel3,  142 },
   { &sHighScore,    139 },
   { &sCredits,      143 },
   { &sTotalPlays,   144 },
   { &sTotalReplays, 145 },
   { &sHiscrBeat,    146 },
   { &sChute2Coins,  147 },
   { &sChute1Coins,  148 },
   { &sChute3Coins,  149 },
   { &sBoot,         138 },
};

static constexpr uint8_t kNumAdjustments = sizeof(kAdjustments) / sizeof(kAdjustments[0]);

// ---------------------------------------------------------------------------
// StoredAdjustmentsMode
// ---------------------------------------------------------------------------

void StoredAdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = 1;
   stateChanged_  = true;
}

void StoredAdjustmentsMode::exit() {}

TopState StoredAdjustmentsMode::update(unsigned long currentTime) {
   bool    curStateChanged = stateChanged_;
   stateChanged_           = false;
   uint8_t curState        = internalState_;
   uint8_t returnState     = curState;

   bool    resetDoubleClick = false;
   uint8_t curSwitch        = machine_->pullFirstFromSwitchStack();

   if (curSwitch == SW_CREDIT_RESET) {
      resetHold_ = currentTime;
      if ((currentTime - lastResetPress_) < 400) {
         resetDoubleClick = true;
         curSwitch        = 0xFF;
      }
      lastResetPress_ = currentTime;
   }

   AdjEntry& entry = kAdjustments[curState - 1];

   if (resetHold_ != 0 && !machine_->readSingleSwitchState(SW_CREDIT_RESET)) {
      resetHold_ = 0;
      entry.adj->onHeldReleased(*machine_);
   }

   bool resetBeingHeld = (resetHold_ != 0 && (currentTime - resetHold_) > 1300);

   if (curSwitch == SW_SLAM) {
      return TopState::Attract;
   }

   if (curSwitch == SW_SELF_TEST_SWITCH &&
       (currentTime - selfTestLastPressedTime_) > 250) {
      returnState += 1;
      selfTestLastPressedTime_ = currentTime;
   }

   // 0 is reserved for "exit to Attract" (e.g. from Boot); wrap back to start.
   if (returnState == 0 && curState != 0) {
      returnState = 1;
   }

   if (curStateChanged) {
      machine_->setCoinLockout(false);
      for (int i = 0; i < 4; i++) {
         machine_->setDisplay(i, 0);
         machine_->setDisplayBlank(i, 0x00);
      }
      machine_->setDisplayCredits(0, false);
      machine_->setDisplayBallInPlay(curState);
      machine_->stopAllAudio();
      machine_->playCallout(entry.callout);
      entry.adj->onEnter(*machine_);
   }

   if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
      if (entry.adj->exitsOnPress()) {
         returnState = 0;
      } else {
         entry.adj->onPress(*machine_, resetDoubleClick);
      }
   }

   if (resetBeingHeld) {
      entry.adj->onHeld(*machine_, currentTime);
   }

   entry.adj->onTick(*machine_, currentTime);

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }

   if (internalState_ == 0) {
      return TopState::Attract;
   }
   if (internalState_ > kNumAdjustments) {
      return TopState::Adjustments;
   }
   return TopState::StoredAdjustments;
}
