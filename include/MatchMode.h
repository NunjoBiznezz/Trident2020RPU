/**************************************************************************
 * MatchMode.h
 *
 * Post-game match sequence. Spins a digit on the ball-in-play display
 * and awards a free credit to any player whose score ends in that digit.
 * Shared by all rule sets — reads final scores from PinballMachine.
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "PinballMachine.h"
#include <stdint.h>

class MatchMode : public MachineMode {
public:
   void setDependencies(PinballMachine& m) { machine_ = &m; }

   void     enter(unsigned long currentTime) override;
   void     exit() override {}
   TopState update(unsigned long currentTime) override;

private:
   PinballMachine* machine_ = nullptr;

   unsigned long CurrentTime            = 0;
   unsigned long MatchSequenceStartTime = 0;
   unsigned long MatchDelay             = 0;
   uint8_t       MatchDigit             = 0;
   uint8_t       NumMatchSpins          = 0;
   uint8_t       ScoreMatches           = 0;
};
