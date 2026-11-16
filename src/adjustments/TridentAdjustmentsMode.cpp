/**************************************************************************
 * TridentAdjustmentsMode.cpp
 **************************************************************************/

#include "../../include/adjustments/TridentAdjustmentsMode.h"
#include "adjustments/AdjustmentTypes.h"
#include "RPU.h"
#include "Trident2020.h"

// ---------------------------------------------------------------------------
// File-static adjustment objects
// ---------------------------------------------------------------------------

static ListByteAdjustment          sBallsOverride;
static MinMaxDefaultByteAdjustment sAwardOverride;
static ScoreAdjustment             sScoreLevel1;
static ScoreAdjustment             sScoreLevel2;
static ScoreAdjustment             sScoreLevel3;
static ScoreAdjustment             sHighScore;
static AuditAdjustment<uint32_t>   sHiscrBeat;
static ScoreULAdjustment           sExtraBallAward;
static MinMaxByteAdjustment        sSpecialAward;
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
   &sHstdFeature,
};

static constexpr uint8_t kNumAdjustments = sizeof(kAdjustments) / sizeof(kAdjustments[0]);

// ---------------------------------------------------------------------------
// GameAdjustmentsMode overrides
// ---------------------------------------------------------------------------

uint8_t TridentAdjustmentsMode::adjustmentCount() const { return kNumAdjustments; }

StoredAdjustment* TridentAdjustmentsMode::getAdjustment(uint8_t i) { return kAdjustments[i]; }

TopState TridentAdjustmentsMode::activeState()    const { return TopState::TridentAdjustments; }
TopState TridentAdjustmentsMode::completedState() const { return TopState::Trident2020Adjustments; }

void TridentAdjustmentsMode::onExit() { game_->readSettings(); }

// ---------------------------------------------------------------------------
// Dependency injection
// ---------------------------------------------------------------------------

void TridentAdjustmentsMode::setDependencies(TridentGame& game, PinballMachine& machine) {
   game_    = &game;
   setMachine(machine);

   TridentGameSettings& s = game.getSettings();

   sBallsOverride.init    (&s.ballsPerGame,        EEPROM_ORIGINAL_BALLS_OVERRIDE_BYTE,      kBallsValues,           3);
   sAwardOverride.init    (&s.scoreAwardReplay,     EEPROM_ORIGINAL_AWARD_OVERRIDE_BYTE,      0, 7);
   sScoreLevel1.init      (&s.awardScores[0],       EEPROM_ORIGINAL_AWARD_SCORE_1_BYTE);
   sScoreLevel2.init      (&s.awardScores[1],       EEPROM_ORIGINAL_AWARD_SCORE_2_BYTE);
   sScoreLevel3.init      (&s.awardScores[2],       EEPROM_ORIGINAL_AWARD_SCORE_3_BYTE);
   sHighScore.init        (&s.highScore,             EEPROM_ORIGINAL_HIGHSCORE_BYTE);
   sHiscrBeat.init        (&s.hiscoreBeat,           EEPROM_ORIGINAL_HISCORE_BEAT_BYTE);
   sExtraBallAward.init   (&s.extraBallValue,        EEPROM_ORIGINAL_EXTRA_BALL_SCORE_BYTE);
   sSpecialAward.init     (&s.specialAward,          EEPROM_ORIGINAL_SPECIAL_SCORE_BYTE,      0, 3);
   sDropTargetSpecial.init(&s.dropTargetSpecialAt,   EEPROM_ORIGINAL_DROP_TARGET_SPECIAL_BYTE, kDropTargetSpecialValues, 2);
   sHighScoreFeature.init (&s.highScoreFeature,      EEPROM_ORIGINAL_HIGH_SCORE_FEATURE_BYTE,  0, 1);
   sMelodyOption.init     (&s.melodyOption,           EEPROM_ORIGINAL_MELODY_OPTION_BYTE,        0, 1);
   sHstdFeature.init      (&s.hstdFeature,            EEPROM_ORIGINAL_HSTD_FEATURE_BYTE,         0, 3);
}
