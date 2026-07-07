/**************************************************************************
 * AttractMode.h
 *
 * Top-level attract mode state. Cycles lamp displays and waits for the
 * start button; transitions to Game when a player is added.
 **************************************************************************/

#pragma once
#include "MachineMode.h"
#include "MachineSettings.h"
#include "PinballMachine.h"
#include "Trident2020Game.h"
#include <stdint.h>

class AttractMode : public MachineMode {
   uint8_t          lastHeadMode_      = 255;
   uint8_t          lastPlayfieldMode_ = 255;
   uint8_t          animNum_           = 0;
   uint8_t          animStep_          = 0;
   unsigned long    lastAnimFrameTime_ = 0;

   Trident2020Game* game_     = nullptr;
   PinballMachine*  machine_  = nullptr;
   MachineSettings* settings_ = nullptr;

   // Firmware version shown on the score displays at startup.
   unsigned long versionMajor_ = 0;
   unsigned long versionMinor_ = 0;
   unsigned long rpuMajor_     = 0;
   unsigned long rpuMinor_     = 0;

public:
   void setDependencies(Trident2020Game& game, PinballMachine& machine, MachineSettings& s) {
      game_     = &game;
      machine_  = &machine;
      settings_ = &s;
   }

   void setVersionInfo(unsigned long major, unsigned long minor,
                       unsigned long rpuMajor, unsigned long rpuMinor) {
      versionMajor_ = major;
      versionMinor_ = minor;
      rpuMajor_     = rpuMajor;
      rpuMinor_     = rpuMinor;
   }

   void     enter(unsigned long currentTime) override;
   TopState update(unsigned long currentTime) override;
};
