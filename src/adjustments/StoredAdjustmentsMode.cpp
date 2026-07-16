/**************************************************************************
 * StoredAdjustmentsMode.cpp
 **************************************************************************/

#include "../../include/adjustments/StoredAdjustmentsMode.h"
#include "adjustments/AdjustmentTypes.h"
#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"

// ---------------------------------------------------------------------------
// Adjustment objects — heterogeneous types require separate named variables;
// the dispatch table below binds each one to its entry callout index.
// All objects use two-phase init via setDependencies().
// ---------------------------------------------------------------------------

static MinMaxByteAdjustment     sCredits;
static AuditAdjustment<uint32_t> sTotalPlays;
static AuditAdjustment<uint32_t> sTotalReplays;
static AuditAdjustment<uint32_t> sChute2Coins;
static AuditAdjustment<uint32_t> sChute1Coins;
static AuditAdjustment<uint32_t> sChute3Coins;
static BootAdjustment            sBoot;

struct AdjEntry {
   StoredAdjustment* adj;
   uint8_t           callout;
};

static AdjEntry kAdjustments[] = {
   { &sCredits,      143 },
   { &sTotalPlays,   144 },
   { &sTotalReplays, 145 },
   { &sChute2Coins,  147 },
   { &sChute1Coins,  148 },
   { &sChute3Coins,  149 },
   { &sBoot,         138 },
};

static constexpr uint8_t kNumAdjustments = sizeof(kAdjustments) / sizeof(kAdjustments[0]);

// ---------------------------------------------------------------------------
// StoredAdjustmentsMode
// ---------------------------------------------------------------------------

void StoredAdjustmentsMode::setDependencies(PinballMachine& machine) {
   machine_ = &machine;
   MachineSettings& s = machine.getSettings();
   sCredits.init     (&s.credits,       EEPROM_CREDITS_BYTE,      0, 99);
   sTotalPlays.init  (&s.totalPlays,    EEPROM_TOTAL_PLAYS_BYTE);
   sTotalReplays.init(&s.totalReplays,  EEPROM_TOTAL_REPLAYS_BYTE);
   sChute2Coins.init (&s.chute2Coins,   EEPROM_CHUTE_2_COINS_BYTE);
   sChute1Coins.init (&s.chute1Coins,   EEPROM_CHUTE_1_COINS_BYTE);
   sChute3Coins.init (&s.chute3Coins,   EEPROM_CHUTE_3_COINS_BYTE);
}

void StoredAdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = 1;
   stateChanged_  = true;
}

void StoredAdjustmentsMode::exit() {
   machine_->readStoredParameters();
}

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
