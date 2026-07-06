/**************************************************************************
 * MachineMode.h
 *
 * Abstract base for the three top-level machine modes (SelfTest, Attract,
 * Game). The top-level dispatcher in loop() holds a MachineMode* and calls
 * update() every tick; the return value signals whether a mode transition
 * is needed.
 **************************************************************************/

#pragma once
#include <stdint.h>

// The three top-level states the machine can be in.
enum class TopState : uint8_t { SelfTest, Attract, Game };

class MachineMode {
public:
   // Called once when this mode becomes active.
   virtual void enter(unsigned long currentTime) { (void)currentTime; }

   // Called once just before leaving this mode.
   virtual void exit() {}

   // Called every loop() tick. Returns the desired top-level state;
   // returning a state other than the current one triggers a transition.
   virtual TopState update(unsigned long currentTime) = 0;
};
