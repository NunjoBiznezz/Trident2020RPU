/**************************************************************************
 * Trident2020AdjustmentsMode.cpp
 **************************************************************************/

#include "../../include/adjustments/Trident2020AdjustmentsMode.h"
#include "adjustments/AdjustmentTypes.h"
#include "RPU.h"
#include "Trident2020.h"

// ---------------------------------------------------------------------------
// Value lists for LIST-type adjustments
// ---------------------------------------------------------------------------

static const uint8_t kBallsValues[]    = { 3, 5, 99 };
static const uint8_t kBallSaveValues[] = { 0, 5, 10, 15, 20 };

// ---------------------------------------------------------------------------
// File-static adjustment objects
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
static ListByteAdjustment          sBallSave;

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
   &sBallSave,
};

static constexpr uint8_t kNumAdjustments = sizeof(kAdjustments) / sizeof(kAdjustments[0]);

// ---------------------------------------------------------------------------
// GameAdjustmentsMode overrides
// ---------------------------------------------------------------------------

uint8_t Trident2020AdjustmentsMode::adjustmentCount() const { return kNumAdjustments; }

StoredAdjustment* Trident2020AdjustmentsMode::getAdjustment(uint8_t i) { return kAdjustments[i]; }

TopState Trident2020AdjustmentsMode::activeState()    const { return TopState::Trident2020Adjustments; }
TopState Trident2020AdjustmentsMode::completedState() const { return TopState::Attract; }

void Trident2020AdjustmentsMode::onUpdate() { game_->setNumPlayers(0); }
void Trident2020AdjustmentsMode::onExit()   { game_->readSettings(); }

// ---------------------------------------------------------------------------
// Dependency injection
// ---------------------------------------------------------------------------

void Trident2020AdjustmentsMode::setDependencies(Trident2020Game& game, PinballMachine& machine) {
   game_    = &game;
   setMachine(machine);

   Trident2020GameSettings& s = game.getSettings();

   sAwardOverride.init      (&s.scoreAwardReplay,        EEPROM_AWARD_OVERRIDE_BYTE,            0, 7);
   sScoreLevel1.init        (&s.awardScores[0],           EEPROM_AWARD_SCORE_1_BYTE);
   sScoreLevel2.init        (&s.awardScores[1],           EEPROM_AWARD_SCORE_2_BYTE);
   sScoreLevel3.init        (&s.awardScores[2],           EEPROM_AWARD_SCORE_3_BYTE);
   sHighScore.init          (&s.highScore,                 EEPROM_HIGHSCORE_BYTE);
   sHiscrBeat.init          (&s.hiscoreBeat,              EEPROM_HISCORE_BEAT_BYTE);
   sBallsOverride.init      (&s.ballsPerGame,             EEPROM_BALLS_OVERRIDE_BYTE,            kBallsValues, 3);
   sExtraBallAward.init     (&s.extraBallValue,            EEPROM_EXTRA_BALL_SCORE_BYTE);
   sSpecialAward.init       (&s.specialValue,              EEPROM_SPECIAL_SCORE_BYTE);
   sSharpShooterBonus.init  (&s.sharpShooterStartBonus,   EEPROM_SHARP_SHOOTER_START_BONUS_BYTE, 1, 5);
   sTargetSpecialBonus.init (&s.targetSpecialBonus,       EEPROM_TARGET_SPECIAL_BONUS_BYTE,      1, 5);
   sStandupSpecialLevel.init(&s.standupSpecialLevel,      EEPROM_STANDUP_SPECIAL_LEVEL_BYTE,     1, 4);
   sBallSave.init            (&s.ballSaveNumSeconds,       EEPROM_BALL_SAVE_BYTE,                 kBallSaveValues, 5);
}
