/**************************************************************************
 * Trident2020Game.h
 *
 * Game rules class for Trident 2020. Encapsulates all state and logic
 * for the Trident 2020 rules set. Designed to be the base for a future
 * Trident class hierarchy (original rules vs. 2020 rules).
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "MachineSettings.h"
#include "PinballMachine.h"
#include <stdint.h>

class Trident2020Game : public MachineMode {
public:
   Trident2020Game();

   // Set once from main.cpp after construction; both remain valid for the
   // lifetime of the program.
   void setMachine(PinballMachine& m) {
      machine_ = &m;
      settings_ = &m.getSettings();
   }

   // MachineMode interface
   void     enter(unsigned long currentTime) override;
   void     exit() override {}
   TopState update(unsigned long currentTime) override;

   // Called from AttractMode and internally.
   // Uses the MachineSettings already provided via setSettings().
   bool addPlayer(bool resetNumPlayers);

   // Score/player accessors for attract mode and self-test
   unsigned long getScore(uint8_t player) const         { return CurrentScores[player]; }
   unsigned long getCurrentPlayerScore() const          { return CurrentPlayerCurrentScore; }
   uint8_t       getCurrentPlayer() const               { return CurrentPlayer; }
   uint8_t       getNumPlayers() const                  { return CurrentNumPlayers; }
   void          setScore(uint8_t p, unsigned long v)      { CurrentScores[p] = v; }
   void          setCurrentPlayerScore(unsigned long v)   { CurrentPlayerCurrentScore = v; }
   void          setNumPlayers(uint8_t n)                 { CurrentNumPlayers = n; }
   void          setGameMode(uint8_t mode)                { GameMode = mode; }
   void          setCurrentTime(unsigned long t)          { CurrentTime = t; }
   void          markScoreChanged(unsigned long t)        { LastTimeScoreChanged = t; }
   uint8_t       getRolloverValue() const                 { return RolloverValue; }

   // Display/lamp methods called from attract mode
   void showPlayerScores(uint8_t displayToUpdate, bool flashCurrent, bool dashCurrent,
                        unsigned long allScoresShowValue = 0);
   void setPlayerLamps(uint8_t numPlayers, uint8_t playerOffset = 0, int flashPeriod = 0);
   void showBonusOnTree(uint8_t bonus, uint8_t dim = 0);
   void showSaucerLamps();
   void showDropTargetLamps();
   void showStandupTargetLamps();
   void showLeftSpinnerLamps();
   void showLeftLaneLamps();

private:
   // Set once via setSettings() / setMachine(); valid for the lifetime of the program.
   MachineSettings*  settings_     = nullptr;
   PinballMachine* machine_ = nullptr;

   // Top-level state machine: internal game state and change flag.
   int  internalState_    = 0;
   bool internalStateChanged_ = false;

   unsigned long CurrentTime = 0;

   // Per-game / per-ball state
   uint8_t       CurrentPlayer = 0;
   uint8_t       CurrentBallInPlay = 1;
   uint8_t       CurrentNumPlayers = 0;
   unsigned long CurrentScores[4] = {};
   unsigned long CurrentPlayerCurrentScore = 0;

   uint8_t Bonus = 0;
   uint8_t BonusX = 0;
   uint8_t StandupsHit[4] = {};
   uint8_t CurrentStandupsHit = 0;
   bool    SamePlayerShootsAgain = false;

   bool          BallSaveUsed = false;
   unsigned long BallFirstSwitchHitTime = 0;
   unsigned long BallTimeInTrough = 0;

   bool    RescueFromTheDeepAvailable = true;
   bool    JackpotLit = false;
   bool    ExtraBallCollected = false;
   bool    SpecialCollected = false;
   bool    ShowingModeStats = false;
   uint8_t GameMode = 0;
   uint8_t NumTiltWarnings = 0;
   uint8_t SaucerValue = 0;
   uint8_t ShowSaucerHit = 0;
   uint8_t LastStandupTargetHit = 0;
   uint8_t CurrentDropTargetsValid = 0;
   uint8_t RolloverValue = 2;
   uint8_t LastSpinnerSide = 0;
   uint8_t AlternatingSpinnerCount = 0;
   uint8_t FeedingFrenzySpins[4] = {};
   uint8_t ExploreTheDepthsHits[4] = {};
   uint8_t SharpShooterHits[4] = {};
   uint8_t CurrentFeedingFrenzy = 0;
   uint8_t CurrentExploreTheDepths = 0;
   uint8_t CurrentSharpShooter = 0;
   uint8_t SharpShooterTarget = 0;
   uint8_t NumberOfStandupClears = 0;
   uint8_t GameModeFlagsQualified = 0;
   bool    PlayerUpLightBlinking = false;
   uint8_t LastBonusShown = 0;

   unsigned long LastSpinnerHitTime = 0;
   unsigned long GameModeStartTime = 0;
   unsigned long GameModeEndTime = 0;
   unsigned long LastTiltWarningTime = 0;
   unsigned long NextSaucerReduction = 0;
   unsigned long SaucerHitTime = 0;
   unsigned long DropTargetClearTime = 0;
   unsigned long StandupDisplayEndTime = 0;
   unsigned long RolloverFlashEndTime = 0;
   unsigned long RescueFromTheDeepEndTime = 0;
   unsigned long LastMiniGameBonusTime = 0;

   // Display management state
   unsigned long LastTimeScoreChanged      = 0;
   unsigned long ScoreOverrideValue[4]     = {};
   uint8_t       ScoreOverrideStatus       = 0;
   unsigned long lastTimeOverrideAnimated_ = 0;
   int           LastReportedValue = 0;

   // Bonus countdown state
   unsigned long CountdownStartTime = 0;
   unsigned long LastCountdownReportTime = 0;
   unsigned long BonusCountDownEndTime = 0;

   // Match sequence state
   unsigned long MatchSequenceStartTime = 0;
   unsigned long MatchDelay = 150;
   uint8_t       MatchDigit = 0;
   uint8_t       NumMatchSpins = 0;
   uint8_t       ScoreMatches = 0;

   // --- Private helpers ---
   void    overrideScoreDisplay(uint8_t displayNum, unsigned long value, bool animate);

   void showBonusLamps();
   void showBonusXLamps();
   void showRightSpinnerLamps();
   void showAwardLamps();
   void showShootAgainLamp();

   uint8_t countBits(uint8_t byteToBeCounted);
   void    resetDropTargets();
   void    handleDropTargetHit(uint8_t switchHit, unsigned long scoreMultiplier);
   void    handleStandupHit(uint8_t switchHit, unsigned long scoreMultiplier);

   int  initGamePlay();
   int  initNewBall(bool curStateChanged, uint8_t playerNum, int ballNum);
   void addToBonus(uint8_t numToAdd);
   void checkForFeedingFrenzyQualify();
   int  manageGameMode();
   int  countdownBonus(bool curStateChanged);
   void checkHighScores();
   int  showMatchSequence(bool curStateChanged);
   void startBallBackgroundSong(uint8_t ballNum);
};
