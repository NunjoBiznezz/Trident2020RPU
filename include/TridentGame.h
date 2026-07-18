/**************************************************************************
 * TridentGame.h
 *
 * Game shell for Original Trident rules. Manages ball play, player
 * rotation, and the match sequence. Scoring and rule-specific behaviour
 * are placeholder stubs to be filled in later.
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "MachineSettings.h"
#include "PinballMachine.h"
#include <stdint.h>

class TridentGame : public MachineMode {
public:
   TridentGame();

   void setMachine(PinballMachine& m) { machine_ = &m; settings_ = &m.getSettings(); }

   void     enter(unsigned long currentTime) override;
   void     exit() override {}
   TopState update(unsigned long currentTime) override;

   bool addPlayer(bool resetNumPlayers);

   unsigned long getScore(uint8_t player) const        { return CurrentScores[player]; }
   unsigned long getCurrentPlayerScore() const         { return CurrentPlayerCurrentScore; }
   uint8_t       getCurrentPlayer() const              { return CurrentPlayer; }
   uint8_t       getNumPlayers() const                 { return CurrentNumPlayers; }

private:
   // Internal substates — private to this class; not shared with other modes.
   static constexpr int kInitGameplay   =   1;
   static constexpr int kInitNewBall    =   2;
   static constexpr int kNormalGameplay =   4;
   static constexpr int kBallOver       = 100;
   static constexpr int kMatchMode      = 110;

   const MachineSettings* settings_ = nullptr;
   PinballMachine*        machine_  = nullptr;

   int  internalState_        = kInitGameplay;
   bool internalStateChanged_ = false;

   unsigned long CurrentTime              = 0;

   uint8_t       CurrentPlayer            = 0;
   uint8_t       CurrentBallInPlay        = 1;
   uint8_t       CurrentNumPlayers        = 0;
   unsigned long CurrentScores[4]         = {};
   unsigned long CurrentPlayerCurrentScore = 0;

   bool          SamePlayerShootsAgain    = false;
   bool          HighScoreBeaten[4]       = {};
   bool          BallSaveUsed             = false;
   unsigned long BallFirstSwitchHitTime   = 0;
   uint8_t       NumTiltWarnings          = 0;
   unsigned long LastTiltWarningTime      = 0;

   uint8_t       CurrentBonusValue        = 1;  // bonus positions 1–19
   unsigned long BonusCountdownTime       = 0;

   // Left lane value progression (rollover advances, left inlane collects)
   uint8_t       LeftLaneValueIndex       = 0;  // 0–3 → 2K/4K/6K/8K

   // Standup target completion state
   uint8_t       StandupTargetsMask      = 0;     // bit i = target i lit this round
   uint8_t       StandupCompletions      = 0;     // times all 5 completed this ball
   bool          StandupExtraBallAvailable = false;

   // Spinner values (per-spin score) and bonus tracking
   uint8_t       SpinnerBonusMask        = 0;     // bit i = spinner bonus for target i awarded this ball
   uint16_t      LeftSpinnerValue        = 200;   // 200 base + 400 per lit left-side target
   uint16_t      RightSpinnerValue       = 200;   // 200 base + 400 per lit right-side target

   // Drop-target multiplier state
   uint8_t       DropTargetsMask          = 0;    // bit i = drop target (i+1) is UP
   uint8_t       BonusMultiplier          = 1;    // current multiplier level (1–5)
   unsigned long CurrentSaucerValue       = 0;    // current top-eject award
   bool          DropTargetSpecialAvailable = false;  // outlane special can be collected

   // kBallOver multi-pass countdown state
   uint8_t       SavedBonusValue          = 1;
   uint8_t       BonusCountdownPassesLeft = 0;

   void setPlayerLamps(uint8_t numPlayers, uint8_t playerOffset = 0, int flashPeriod = 0);
   void showPlayerScores(uint8_t displayToUpdate, bool flashCurrent, bool dashCurrent,
                         unsigned long allScoresShowValue = 0);

   int  initGamePlay();
   int  initNewBall(bool curStateChanged, uint8_t playerNum, int ballNum);
   void startBallBackgroundSong(uint8_t ballNum);

   void advanceBonus(uint8_t positions = 1);
   void showBonusLamps();
   void setupDropTargets(uint8_t mask);
};
