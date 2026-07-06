/**************************************************************************
 * Trident2020Game.h
 *
 * Game rules class for Trident 2020. Encapsulates all state and logic
 * for the Trident 2020 rules set. Designed to be the base for a future
 * Trident class hierarchy (original rules vs. 2020 rules).
 **************************************************************************/

#pragma once
#include <stdint.h>

// Machine-level operator settings and persistent state. Populated in main.cpp
// from EEPROM / ReadStoredParameters() and passed into each run() call so that
// the game class does not need to access statics from another translation unit.
struct GameContext {
   uint8_t*       credits;
   const uint8_t* maximumCredits;
   const uint8_t* ballsPerGame;
   const uint8_t* ballSaveNumSeconds;
   const bool*    freePlayMode;
   const bool*    tournamentScoring;
   const bool*    scrollingScores;
   unsigned long* highScore;
   unsigned long* awardScores;          // points to AwardScores[3]
   const uint8_t* scoreAwardReplay;
   const uint8_t* maxTiltWarnings;
   bool*          resetScoresToClearVersion;
   const unsigned long* extraBallValue;
   const unsigned long* specialValue;
   const bool*    highScoreReplay;
   const bool*    matchFeature;
   const uint8_t* sharpShooterStartBonus;
   const uint8_t* targetSpecialBonus;
   const uint8_t* standupSpecialLevel;
};

class Trident2020Game {
public:
   Trident2020Game();

   // Main entry point — called from loop() instead of RunGamePlayMode()
   int run(int curState, bool curStateChanged, unsigned long currentTime, GameContext& ctx);

   // Called from RunAttractMode() and internally
   bool addPlayer(bool resetNumPlayers, GameContext& ctx);

   // Score/player accessors for attract mode and self-test
   unsigned long getScore(uint8_t player) const         { return CurrentScores[player]; }
   unsigned long getCurrentPlayerScore() const          { return CurrentPlayerCurrentScore; }
   uint8_t       getCurrentPlayer() const               { return CurrentPlayer; }
   uint8_t       getNumPlayers() const                  { return CurrentNumPlayers; }
   void          setScore(uint8_t p, unsigned long v)      { CurrentScores[p] = v; }
   void          setCurrentPlayerScore(unsigned long v)   { CurrentPlayerCurrentScore = v; }
   void          setNumPlayers(uint8_t n)                 { CurrentNumPlayers = n; }
   void          setGameMode(uint8_t mode)                { GameMode = mode; }
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
   // ctx_ is set at the start of run() and remains valid for the entire call stack
   // that originates from run(). It is nullptr outside of run().
   GameContext* ctx_ = nullptr;

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
   unsigned long LastTimeScoreChanged = 0;
   unsigned long LastTimeOverrideAnimated = 0;
   unsigned long LastFlashOrDash = 0;
   unsigned long ScoreOverrideValue[4] = {};
   uint8_t       ScoreOverrideStatus = 0;
   uint8_t       LastScrollPhase = 0;
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
   uint8_t magnitudeOfScore(unsigned long score);
   void    overrideScoreDisplay(uint8_t displayNum, unsigned long value, bool animate);
   uint8_t getDisplayMask(uint8_t numDigits);

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
};
