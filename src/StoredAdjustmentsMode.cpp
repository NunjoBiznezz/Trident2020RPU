/**************************************************************************
 * StoredAdjustmentsMode.cpp
 **************************************************************************/

#include "StoredAdjustmentsMode.h"
#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include "adjustments/Adjustments.h"

// ---------------------------------------------------------------------------
// Setting indices and EEPROM addresses
// ---------------------------------------------------------------------------

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

// Callout index 0 = setting 1 (score level 1) … index 11 = setting 12 (boot).
static const uint8_t kStoredAdjCalloutMap[kSaBoot] = {
   140, 141, 142, 139, 143, 144, 145, 146, 147, 148, 149, 138
};

// ---------------------------------------------------------------------------
// Adjustment objects and dispatch table
// ---------------------------------------------------------------------------

static ScoreAdjustment   sScoreLevel1  { kEeScoreLevel1  };
static ScoreAdjustment   sScoreLevel2  { kEeScoreLevel2  };
static ScoreAdjustment   sScoreLevel3  { kEeScoreLevel3  };
static ScoreAdjustment   sHighScore    { kEeHighScore     };
static CreditsAdjustment sCredits      { kEeCredits       };
static AuditAdjustment   sTotalPlays   { kEeTotalPlays    };
static AuditAdjustment   sTotalReplays { kEeTotalReplays  };
static AuditAdjustment   sHiscrBeat   { kEeHiscrBeat     };
static AuditAdjustment   sChute2Coins { kEeChute2Coins   };
static AuditAdjustment   sChute1Coins { kEeChute1Coins   };
static AuditAdjustment   sChute3Coins { kEeChute3Coins   };
static BootAdjustment    sBoot;

static StoredAdjustment* sAdjustments[kSaBoot] = {
   &sScoreLevel1, &sScoreLevel2, &sScoreLevel3, &sHighScore,
   &sCredits,
   &sTotalPlays, &sTotalReplays, &sHiscrBeat,
   &sChute2Coins, &sChute1Coins, &sChute3Coins,
   &sBoot
};

// ---------------------------------------------------------------------------
// StoredAdjustmentsMode
// ---------------------------------------------------------------------------

void StoredAdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = kSaScoreLevel1;
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

   StoredAdjustment* adj = sAdjustments[curState - 1];

   if (resetHold_ != 0 && !machine_->readSingleSwitchState(SW_CREDIT_RESET)) {
      resetHold_ = 0;
      adj->onHeldReleased(*machine_);
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
      returnState = kSaScoreLevel1;
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
      machine_->playCallout(kStoredAdjCalloutMap[curState - 1]);
      adj->onEnter(*machine_);
   }

   if (curSwitch == SW_CREDIT_RESET || resetDoubleClick) {
      if (adj->exitsOnPress()) {
         returnState = 0;
      } else {
         adj->onPress(*machine_, resetDoubleClick);
      }
   }

   if (resetBeingHeld) {
      adj->onHeld(*machine_, currentTime);
   }

   adj->onTick(*machine_, currentTime);

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }

   if (internalState_ == 0) {
      return TopState::Attract;
   }
   if (internalState_ > kSaBoot) {
      return TopState::Adjustments;
   }
   return TopState::StoredAdjustments;
}
