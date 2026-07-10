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

   void setSettings(const MachineSettings& s) { settings_ = &s; }
   void setMachine(PinballMachine& m)         { machine_ = &m; }

   void     enter(unsigned long currentTime) override;
   void     exit() override {}
   TopState update(unsigned long currentTime) override;

   bool addPlayer(bool resetNumPlayers);

   unsigned long getScore(uint8_t player) const        { return CurrentScores[player]; }
   unsigned long getCurrentPlayerScore() const         { return CurrentPlayerCurrentScore; }
   uint8_t       getCurrentPlayer() const              { return CurrentPlayer; }
   uint8_t       getNumPlayers() const                 { return CurrentNumPlayers; }

private:
   const MachineSettings* settings_ = nullptr;
   PinballMachine*  machine_ = nullptr;

   int  internalState_        = 0;
   bool internalStateChanged_ = false;

   unsigned long CurrentTime              = 0;

   uint8_t       CurrentPlayer            = 0;
   uint8_t       CurrentBallInPlay        = 1;
   uint8_t       CurrentNumPlayers        = 0;
   unsigned long CurrentScores[4]         = {};
   unsigned long CurrentPlayerCurrentScore = 0;

   bool          SamePlayerShootsAgain    = false;
   bool          BallSaveUsed             = false;
   unsigned long BallFirstSwitchHitTime   = 0;
   uint8_t       NumTiltWarnings          = 0;
   unsigned long LastTiltWarningTime      = 0;

   // Match sequence state
   unsigned long MatchSequenceStartTime   = 0;
   unsigned long MatchDelay               = 150;
   uint8_t       MatchDigit               = 0;
   uint8_t       NumMatchSpins            = 0;
   uint8_t       ScoreMatches             = 0;

   void setPlayerLamps(uint8_t numPlayers, uint8_t playerOffset = 0, int flashPeriod = 0);
   void showPlayerScores(uint8_t displayToUpdate, bool flashCurrent, bool dashCurrent,
                         unsigned long allScoresShowValue = 0);

   int  initGamePlay();
   int  initNewBall(bool curStateChanged, uint8_t playerNum, int ballNum);
   int  showMatchSequence(bool curStateChanged);
   void startBallBackgroundSong(uint8_t ballNum);
};
