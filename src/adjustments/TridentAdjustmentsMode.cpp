/**************************************************************************
 * TridentAdjustmentsMode.cpp
 **************************************************************************/

#include "adjustments/TridentAdjustmentsMode.h"
#include "adjustments/AdjustmentTypes.h"
#include "RPU.h"
#include "Trident2020.h"

// ---------------------------------------------------------------------------
// File-static adjustment objects — pointers initialised in setDependencies()
// ---------------------------------------------------------------------------

static ListByteAdjustment          sBallsOverride;
static MinMaxDefaultByteAdjustment sAwardOverride;
static ScoreAdjustment             sScoreLevel1;
static ScoreAdjustment             sScoreLevel2;
static ScoreAdjustment             sScoreLevel3;
static ScoreAdjustment             sHighScore;
static AuditAdjustment<uint32_t>   sHiscrBeat;
static ScoreULAdjustment           sExtraBallAward;
static ScoreULAdjustment           sSpecialAward;
static ListByteAdjustment          sDropTargetSpecial;
static MinMaxByteAdjustment        sHighScoreFeature;
static MinMaxByteAdjustment        sMelodyOption;
static MinMaxByteAdjustment        sHstdFeature;

static const uint8_t kBallsValues[]             = { 3, 5, 99 };
static const uint8_t kDropTargetSpecialValues[] = { 4, 5 };

static StoredAdjustment* kAdjustments[] = {
   &sBallsOverride,
   &sAwardOverride,
   &sScoreLevel1,
   &sScoreLevel2,
   &sScoreLevel3,
   &sHighScore,
   &sHiscrBeat,
   &sExtraBallAward,
   &sSpecialAward,
   &sDropTargetSpecial,
   &sHighScoreFeature,
   &sMelodyOption,
};

static constexpr uint8_t kNumAdjustments = sizeof(kAdjustments) / sizeof(kAdjustments[0]);

// ---------------------------------------------------------------------------
// TridentAdjustmentsMode
// ---------------------------------------------------------------------------

void TridentAdjustmentsMode::setDependencies(PinballMachine& machine) {
   machine_ = &machine;

   MachineSettings& s = machine.getSettings();

   sBallsOverride.init    (&s.tridentSettings.ballsPerGame,       EEPROM_ORIGINAL_BALLS_OVERRIDE_BYTE,      kBallsValues,           3);
   sAwardOverride.init    (&s.tridentSettings.scoreAwardReplay,  EEPROM_ORIGINAL_AWARD_OVERRIDE_BYTE,      0, 7);
   sScoreLevel1.init      (&s.tridentSettings.awardScores[0],    EEPROM_ORIGINAL_AWARD_SCORE_1_BYTE);
   sScoreLevel2.init      (&s.tridentSettings.awardScores[1],    EEPROM_ORIGINAL_AWARD_SCORE_2_BYTE);
   sScoreLevel3.init      (&s.tridentSettings.awardScores[2],    EEPROM_ORIGINAL_AWARD_SCORE_3_BYTE);
   sHighScore.init        (&s.tridentSettings.highScore,          EEPROM_ORIGINAL_HIGHSCORE_BYTE);
   sHiscrBeat.init        (&s.tridentSettings.hiscoreBeat,        EEPROM_ORIGINAL_HISCORE_BEAT_BYTE);
   sExtraBallAward.init   (&s.tridentSettings.extraBallValue,     EEPROM_ORIGINAL_EXTRA_BALL_SCORE_BYTE);
   sSpecialAward.init     (&s.tridentSettings.specialValue,       EEPROM_ORIGINAL_SPECIAL_SCORE_BYTE);
   sDropTargetSpecial.init(&s.tridentSettings.dropTargetSpecialAt, EEPROM_ORIGINAL_DROP_TARGET_SPECIAL_BYTE, kDropTargetSpecialValues, 2);
   sHighScoreFeature.init(&s.tridentSettings.highScoreFeature, EEPROM_ORIGINAL_HIGH_SCORE_FEATURE_BYTE, 0, 1);
   sMelodyOption.init(&s.tridentSettings.melodyOption, EEPROM_ORIGINAL_MELODY_OPTION_BYTE, 0, 1);
   sHstdFeature.init(&s.tridentSettings.hstdFeature, EEPROM_ORIGINAL_HSTD_FEATURE_BYTE, 0, 3);
}

void TridentAdjustmentsMode::enter(unsigned long /*currentTime*/) {
   internalState_ = 1;
   stateChanged_  = true;
}

void TridentAdjustmentsMode::exit() {
   machine_->readStoredParameters();
}

TopState TridentAdjustmentsMode::update(unsigned long currentTime) {
   bool    curStateChanged = stateChanged_;
   stateChanged_           = false;
   uint8_t curState        = internalState_;
   uint8_t returnState     = curState;

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
      return TopState::Trident2020Adjustments;
   }
   return TopState::TridentAdjustments;
}
