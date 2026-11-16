/**************************************************************************
 * StoredAdjustmentsMode.cpp
 **************************************************************************/

#include "adjustments/StoredAdjustmentsMode.h"
#include "adjustments/AdjustmentTypes.h"
#include "MachineEeprom.h"
#include "RPU.h"
#include "Trident.h"


// ListByteAdjustment that previews brightness by blinking the bonus lamps.
class DimLevelAdjustment : public ListByteAdjustment {
 public:
   void onEnter(PinballMachine& machine) override {
      ListByteAdjustment::onEnter(machine);
      for (int i = 0; i < 10; i++) {
         machine.setLampState(LAMP_BONUS_1 + i, true, 1);
      }
   }
   void onPress(PinballMachine& machine, bool doubleClick) override {
      ListByteAdjustment::onPress(machine, doubleClick);
      machine.setDimDivisor(1, *field_);
   }
   void onTick(PinballMachine& machine, unsigned long currentTime) override {
      for (int i = 0; i < 10; i++) {
         machine.setLampState(LAMP_BONUS_1 + i, true, (uint8_t)((currentTime / 1000) % 2));
      }
   }
};

// ---------------------------------------------------------------------------
// Value lists for LIST-type adjustments
// ---------------------------------------------------------------------------

static const uint8_t kDimLevelValues[] = {2, 3};

// ---------------------------------------------------------------------------
// Adjustment objects — heterogeneous types require separate named variables;
// the dispatch table below binds each one to its entry callout index.
// All objects use two-phase init via setDependencies().
// ---------------------------------------------------------------------------

static MinMaxByteAdjustment sFreePlay;
static MinMaxByteAdjustment sMaximumCredits;
static MinMaxByteAdjustment sMatchFeature;
static MinMaxByteAdjustment sHighScoreReplay;
static MinMaxByteAdjustment sActiveRuleSet;
static MinMaxByteAdjustment sSoundSelector;
static MinMaxByteAdjustment sMusicVolume;
static MinMaxByteAdjustment sSfxVolume;
static MinMaxByteAdjustment sCalloutsVolume;
static MinMaxByteAdjustment sTournamentScoring;
static MinMaxByteAdjustment sTiltWarnings;
static MinMaxByteAdjustment sScrollingScores;
static DimLevelAdjustment sDimLevel;
static MinMaxByteAdjustment sCredits;
static AuditAdjustment<uint32_t> sTotalPlays;
static AuditAdjustment<uint32_t> sTotalReplays;
static AuditAdjustment<uint32_t> sChute2Coins;
static AuditAdjustment<uint32_t> sChute1Coins;
static AuditAdjustment<uint32_t> sChute3Coins;
static BootAdjustment sBoot;

static StoredAdjustment* kAdjustments[] = {
    &sFreePlay,   &sMaximumCredits, &sMatchFeature,      &sHighScoreReplay, &sActiveRuleSet,   &sSoundSelector, &sMusicVolume,
    &sSfxVolume,  &sCalloutsVolume, &sTournamentScoring, &sTiltWarnings,    &sScrollingScores, &sDimLevel,      &sCredits,
    &sTotalPlays, &sTotalReplays,   &sChute2Coins,       &sChute1Coins,     &sChute3Coins,     &sBoot,
};

static constexpr uint8_t kNumAdjustments = sizeof(kAdjustments) / sizeof(kAdjustments[0]);

// ---------------------------------------------------------------------------
// StoredAdjustmentsMode
// ---------------------------------------------------------------------------

void StoredAdjustmentsMode::setDependencies(PinballMachine& machine) {
   machine_ = &machine;
   MachineSettings& s = machine.getSettings();

   sFreePlay.init((uint8_t*)&s.freePlayMode, EEPROM_FREE_PLAY_BYTE, 0, 1);
   sMaximumCredits.init(&s.maximumCredits, EEPROM_MAXIMUM_CREDITS_BYTE, 1, 99);
   sMatchFeature.init((uint8_t*)&s.matchFeature, EEPROM_MATCH_FEATURE_BYTE, 0, 1);
   sHighScoreReplay.init((uint8_t*)&s.highScoreReplay, EEPROM_HIGH_SCORE_REPLAY_BYTE, 0, 1);
   sActiveRuleSet.init((uint8_t*)&s.activeRuleSet, EEPROM_ACTIVE_RULE_SET_BYTE, 0, 1);
   sSoundSelector.init(&s.soundSelector, EEPROM_SOUND_SELECTOR_BYTE, 0, 5);
   sMusicVolume.init(&s.musicVolume, EEPROM_MUSIC_VOLUME_BYTE, 0, 10);
   sSfxVolume.init(&s.sfxVolume, EEPROM_SFX_VOLUME_BYTE, 0, 10);
   sCalloutsVolume.init(&s.calloutsVolume, EEPROM_CALLOUTS_VOLUME_BYTE, 0, 10);
   sTournamentScoring.init((uint8_t*)&s.tournamentScoring, EEPROM_TOURNAMENT_SCORING_BYTE, 0, 1);
   sTiltWarnings.init(&s.maxTiltWarnings, EEPROM_TILT_WARNING_BYTE, 0, 2);
   sScrollingScores.init((uint8_t*)&s.scrollingScores, EEPROM_SCROLLING_SCORES_BYTE, 0, 1);
   sDimLevel.init(&s.dimLevel, EEPROM_DIM_LEVEL_BYTE, kDimLevelValues, 2);
   sCredits.init(&s.credits, EEPROM_CREDITS_BYTE, 0, 99);
   sTotalPlays.init(&s.totalPlays, EEPROM_TOTAL_PLAYS_BYTE);
   sTotalReplays.init(&s.totalReplays, EEPROM_TOTAL_REPLAYS_BYTE);
   sChute2Coins.init(&s.chute2Coins, EEPROM_CHUTE_2_COINS_BYTE);
   sChute1Coins.init(&s.chute1Coins, EEPROM_CHUTE_1_COINS_BYTE);
   sChute3Coins.init(&s.chute3Coins, EEPROM_CHUTE_3_COINS_BYTE);
}

void StoredAdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = 1;
   stateChanged_ = true;
}

void StoredAdjustmentsMode::exit() {
   machine_->readStoredParameters();
}

TopState StoredAdjustmentsMode::update(unsigned long currentTime) {
   bool curStateChanged = stateChanged_;
   stateChanged_ = false;
   uint8_t curState = internalState_;
   uint8_t returnState = curState;

   bool resetDoubleClick = false;
   uint8_t curSwitch = machine_->pullFirstFromSwitchStack();

   if (curSwitch == SW_CREDIT_RESET) {
      resetHold_ = currentTime;
      if ((currentTime - lastResetPress_) < 400) {
         resetDoubleClick = true;
         curSwitch = 0xFF;
      }
      lastResetPress_ = currentTime;
   }

   StoredAdjustment* adj = kAdjustments[curState - 1];

   if (resetHold_ != 0 && !machine_->readSingleSwitchState(SW_CREDIT_RESET)) {
      resetHold_ = 0;
      adj->onHeldReleased(*machine_);
   }

   bool resetBeingHeld = (resetHold_ != 0 && (currentTime - resetHold_) > 1300);

   if (curSwitch == SW_SLAM) {
      return TopState::Attract;
   }

   if (curSwitch == SW_SELF_TEST_SWITCH && (currentTime - selfTestLastPressedTime_) > 250) {
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
      stateChanged_ = true;
   }

   if (internalState_ == 0) {
      return TopState::Attract;
   }
   if (internalState_ > kNumAdjustments) {
      return TopState::TridentAdjustments;
   }
   return TopState::StoredAdjustments;
}
