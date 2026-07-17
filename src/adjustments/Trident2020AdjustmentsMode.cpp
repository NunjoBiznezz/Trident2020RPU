/**************************************************************************
 * Trident2020AdjustmentsMode.cpp
 **************************************************************************/

#include "../../include/adjustments/Trident2020AdjustmentsMode.h"
#include "adjustments/AdjustmentTypes.h"
#include "RPU.h"
#include "Trident2020.h"
#include <EEPROM.h>

// ---------------------------------------------------------------------------
// Value lists for LIST-type adjustments
// ---------------------------------------------------------------------------

static const uint8_t kBallsValues[] = { 3, 5, 99 };

// ---------------------------------------------------------------------------
// File-static adjustment objects — pointers initialised in setDependencies()
// ---------------------------------------------------------------------------

static MinMaxDefaultByteAdjustment sAwardOverride;
static ScoreAdjustment             sScoreLevel1;
static ScoreAdjustment             sScoreLevel2;
static ScoreAdjustment             sScoreLevel3;
static ScoreAdjustment             sHighScore;
static AuditAdjustment<uint32_t>   sHiscrBeat;
static ListByteAdjustment          sBallsOverride;
static ScoreULAdjustment           sExtraBallAward;
static ScoreULAdjustment           sSpecialAward;
static MinMaxByteAdjustment        sSharpShooterBonus;
static MinMaxByteAdjustment        sTargetSpecialBonus;
static MinMaxByteAdjustment        sStandupSpecialLevel;

static StoredAdjustment* kAdjustments[] = {
   &sAwardOverride,
   &sScoreLevel1,
   &sScoreLevel2,
   &sScoreLevel3,
   &sHighScore,
   &sHiscrBeat,
   &sBallsOverride,
   &sExtraBallAward,
   &sSpecialAward,
   &sSharpShooterBonus,
   &sTargetSpecialBonus,
   &sStandupSpecialLevel,
};

static constexpr uint8_t kNumAdjustments = sizeof(kAdjustments) / sizeof(kAdjustments[0]);

// ---------------------------------------------------------------------------
// Trident2020AdjustmentsMode
// ---------------------------------------------------------------------------

void Trident2020AdjustmentsMode::setDependencies(Trident2020Game& game, PinballMachine& machine) {
   game_    = &game;
   machine_ = &machine;

   MachineSettings& s = machine.getSettings();

   sAwardOverride.init      (&s.trident2020Settings.scoreAwardReplay,        EEPROM_AWARD_OVERRIDE_BYTE,            0, 7);
   sScoreLevel1.init        (&s.trident2020Settings.awardScores[0],           EEPROM_AWARD_SCORE_1_BYTE);
   sScoreLevel2.init        (&s.trident2020Settings.awardScores[1],           EEPROM_AWARD_SCORE_2_BYTE);
   sScoreLevel3.init        (&s.trident2020Settings.awardScores[2],           EEPROM_AWARD_SCORE_3_BYTE);
   sHighScore.init          (&s.trident2020Settings.highScore,                 EEPROM_HIGHSCORE_BYTE);
   sHiscrBeat.init          (&s.trident2020Settings.hiscoreBeat,              EEPROM_HISCORE_BEAT_BYTE);
   sBallsOverride.init      (&s.trident2020Settings.ballsPerGame,             EEPROM_BALLS_OVERRIDE_BYTE,            kBallsValues, 3);
   sExtraBallAward.init     (&s.trident2020Settings.extraBallValue,            EEPROM_EXTRA_BALL_SCORE_BYTE);
   sSpecialAward.init       (&s.trident2020Settings.specialValue,              EEPROM_SPECIAL_SCORE_BYTE);
   sSharpShooterBonus.init  (&s.trident2020Settings.sharpShooterStartBonus,   EEPROM_SHARP_SHOOTER_START_BONUS_BYTE, 1, 5);
   sTargetSpecialBonus.init (&s.trident2020Settings.targetSpecialBonus,       EEPROM_TARGET_SPECIAL_BONUS_BYTE,      1, 5);
   sStandupSpecialLevel.init(&s.trident2020Settings.standupSpecialLevel,      EEPROM_STANDUP_SPECIAL_LEVEL_BYTE,     1, 4);
}

void Trident2020AdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = 1;
   stateChanged_  = true;
}

void Trident2020AdjustmentsMode::exit() {
   machine_->readStoredParameters();
}

TopState Trident2020AdjustmentsMode::update(unsigned long currentTime) {
   bool    curStateChanged = stateChanged_;
   stateChanged_           = false;
   uint8_t curState        = internalState_;
   uint8_t returnState     = curState;

   game_->setNumPlayers(0);

   uint8_t curSwitch = machine_->pullFirstFromSwitchStack();

   if (curSwitch == SW_SELF_TEST_SWITCH &&
       (currentTime - selfTestLastPressedTime_) > 250) {
      selfTestLastPressedTime_ = currentTime;
      returnState += 1;
   }

   if (curSwitch == SW_SLAM) {
      returnState = 0;
   }

   if (curState >= 1 && curState <= kNumAdjustments) {
      StoredAdjustment* adj = kAdjustments[curState - 1];

      if (curStateChanged) {
         machine_->stopAllAudio();
         for (int i = 0; i < 4; i++) {
            machine_->setDisplay(i, 0);
            machine_->setDisplayBlank(i, 0x00);
         }
         machine_->setDisplayCredits(curState, true);
         machine_->setDisplayBallInPlay(0, false);
         adj->onEnter(*machine_);
      }

      if (curSwitch == SW_CREDIT_RESET) {
         adj->onPress(*machine_, false);
      }

      adj->onTick(*machine_, currentTime);
   }

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }
   if (internalState_ == 0 || internalState_ > kNumAdjustments) {
      return TopState::Attract;
   }
   return TopState::Trident2020Adjustments;
}
