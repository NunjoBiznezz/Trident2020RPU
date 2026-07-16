/**************************************************************************
 * Trident2020AdjustmentsMode.cpp
 **************************************************************************/

#include "../../include/adjustments/Trident2020AdjustmentsMode.h"
#include "adjustments/AdjustmentTypes.h"
#include "RPU.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include <EEPROM.h>

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

static const uint8_t kBallSaveValues[] = { 0, 5, 10, 15, 20 };
static const uint8_t kBallsValues[]    = { 3, 5, 99 };
static const uint8_t kDimLevelValues[] = { 2, 3 };

// ---------------------------------------------------------------------------
// File-static adjustment objects — pointers initialised in setDependencies()
// ---------------------------------------------------------------------------

static MinMaxByteAdjustment        sFreePlay;
static ListByteAdjustment          sBallSave;
static MinMaxByteAdjustment        sSoundSelector;
static MinMaxByteAdjustment        sMusicVolume;
static MinMaxByteAdjustment        sSfxVolume;
static MinMaxByteAdjustment        sCalloutsVolume;
static MinMaxByteAdjustment        sTournamentScoring;
static MinMaxByteAdjustment        sTiltWarnings;
static MinMaxDefaultByteAdjustment sAwardOverride;
static ScoreAdjustment             sScoreLevel1;
static ScoreAdjustment             sScoreLevel2;
static ScoreAdjustment             sScoreLevel3;
static ScoreAdjustment             sHighScore;
static AuditAdjustment<uint32_t>   sHiscrBeat;
static ListByteAdjustment          sBallsOverride;
static MinMaxByteAdjustment        sScrollingScores;
static ScoreULAdjustment           sExtraBallAward;
static ScoreULAdjustment           sSpecialAward;
static DimLevelAdjustment          sDimLevel;
static MinMaxByteAdjustment        sSharpShooterBonus;
static MinMaxByteAdjustment        sTargetSpecialBonus;
static MinMaxByteAdjustment        sStandupSpecialLevel;

struct AdjEntry {
   StoredAdjustment* adj;
   uint8_t           callout;
};

static AdjEntry kAdjustments[] = {
   { &sFreePlay,            153 },
   { &sBallSave,            154 },
   { &sSoundSelector,       155 },
   { &sMusicVolume,         156 },
   { &sSfxVolume,           157 },
   { &sCalloutsVolume,      158 },
   { &sTournamentScoring,   159 },
   { &sTiltWarnings,        160 },
   { &sAwardOverride,       161 },
   { &sScoreLevel1,         140 },
   { &sScoreLevel2,         141 },
   { &sScoreLevel3,         142 },
   { &sHighScore,           139 },
   { &sHiscrBeat,           146 },
   { &sBallsOverride,       162 },
   { &sScrollingScores,     163 },
   { &sExtraBallAward,      164 },
   { &sSpecialAward,        165 },
   { &sDimLevel,            171 },
   { &sSharpShooterBonus,   166 },
   { &sTargetSpecialBonus,  167 },
   { &sStandupSpecialLevel, 168 },
};

static constexpr uint8_t kNumAdjustments = sizeof(kAdjustments) / sizeof(kAdjustments[0]);

// ---------------------------------------------------------------------------
// Trident2020AdjustmentsMode
// ---------------------------------------------------------------------------

void Trident2020AdjustmentsMode::setDependencies(Trident2020Game& game, PinballMachine& machine) {
   game_    = &game;
   machine_ = &machine;

   MachineSettings& s = machine.getSettings();

   sFreePlay.init           ((uint8_t*)&s.freePlayMode,                   EEPROM_FREE_PLAY_BYTE,                 0, 1);
   sBallSave.init           (&s.ballSaveNumSeconds,                        EEPROM_BALL_SAVE_BYTE,                 kBallSaveValues,   5);
   sSoundSelector.init      (&s.soundSelector,                             EEPROM_SOUND_SELECTOR_BYTE,            0, 5);
   sMusicVolume.init        (&s.musicVolume,                               EEPROM_MUSIC_VOLUME_BYTE,              0, 10);
   sSfxVolume.init          (&s.sfxVolume,                                 EEPROM_SFX_VOLUME_BYTE,                0, 10);
   sCalloutsVolume.init     (&s.calloutsVolume,                            EEPROM_CALLOUTS_VOLUME_BYTE,           0, 10);
   sTournamentScoring.init  ((uint8_t*)&s.tournamentScoring,              EEPROM_TOURNAMENT_SCORING_BYTE,        0, 1);
   sTiltWarnings.init       (&s.maxTiltWarnings,                           EEPROM_TILT_WARNING_BYTE,              0, 2);
   sAwardOverride.init      (&s.trident2020Awards.scoreAwardReplay,        EEPROM_AWARD_OVERRIDE_BYTE,            0, 7);
   sScoreLevel1.init        (&s.trident2020Awards.awardScores[0],          EEPROM_AWARD_SCORE_1_BYTE);
   sScoreLevel2.init        (&s.trident2020Awards.awardScores[1],          EEPROM_AWARD_SCORE_2_BYTE);
   sScoreLevel3.init        (&s.trident2020Awards.awardScores[2],          EEPROM_AWARD_SCORE_3_BYTE);
   sHighScore.init          (&s.trident2020Awards.highScore,                EEPROM_HIGHSCORE_BYTE);
   sHiscrBeat.init          (&s.hiscoreBeat,                                EEPROM_HISCORE_BEAT_BYTE);
   sBallsOverride.init      (&s.ballsPerGame,                              EEPROM_BALLS_OVERRIDE_BYTE,            kBallsValues,      3);
   sScrollingScores.init    ((uint8_t*)&s.scrollingScores,                EEPROM_SCROLLING_SCORES_BYTE,          0, 1);
   sExtraBallAward.init     (&s.trident2020Awards.extraBallValue,          EEPROM_EXTRA_BALL_SCORE_BYTE);
   sSpecialAward.init       (&s.trident2020Awards.specialValue,            EEPROM_SPECIAL_SCORE_BYTE);
   sDimLevel.init           (&s.dimLevel,                                   EEPROM_DIM_LEVEL_BYTE,                 kDimLevelValues,   2);
   sSharpShooterBonus.init  (&s.trident2020Balance.sharpShooterStartBonus, EEPROM_SHARP_SHOOTER_START_BONUS_BYTE, 1, 5);
   sTargetSpecialBonus.init (&s.trident2020Balance.targetSpecialBonus,     EEPROM_TARGET_SPECIAL_BONUS_BYTE,      1, 5);
   sStandupSpecialLevel.init(&s.trident2020Balance.standupSpecialLevel,    EEPROM_STANDUP_SPECIAL_LEVEL_BYTE,     1, 4);
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
      AdjEntry& entry = kAdjustments[curState - 1];

      if (curStateChanged) {
         machine_->stopAllAudio();
         machine_->playCallout(entry.callout);
         for (int i = 0; i < 4; i++) {
            machine_->setDisplay(i, 0);
            machine_->setDisplayBlank(i, 0x00);
         }
         machine_->setDisplayCredits(curState, true);
         machine_->setDisplayBallInPlay(0, false);
         entry.adj->onEnter(*machine_);
      }

      if (curSwitch == SW_CREDIT_RESET) {
         entry.adj->onPress(*machine_, false);
      }

      entry.adj->onTick(*machine_, currentTime);
   }

   if (returnState != internalState_) {
      internalState_ = returnState;
      stateChanged_  = true;
   }
   if (internalState_ == 0 || internalState_ > kNumAdjustments) {
      return TopState::Attract;
   }
   return TopState::Adjustments;
}
