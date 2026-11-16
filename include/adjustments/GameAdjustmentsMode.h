/**************************************************************************
 * adjustments/GameAdjustmentsMode.h
 *
 * Base class for the per-game operator adjustment menus (Original Trident
 * and Trident 2020). Provides the shared service-menu update loop;
 * derived classes supply the adjustment list, display mode ID, and
 * TopState routing.
 *
 * Display convention:
 *   Ball-in-play  — current step number (1 … N)
 *   Credits       — mode ID (1 = Original Trident, 20 = Trident 2020)
 **************************************************************************/

#pragma once
#include "../MachineMode.h"
#include "../PinballMachine.h"
#include "AdjustmentTypes.h"
#include <stdint.h>

class GameAdjustmentsMode : public MachineMode {
   uint8_t       internalState_           = 0;
   bool          stateChanged_            = false;
   unsigned long selfTestLastPressedTime_ = 0;

protected:
   PinballMachine* machine_ = nullptr;

   // Number of adjustments in the derived class's list.
   virtual uint8_t adjustmentCount() const = 0;

   // Return the adjustment object at the given 0-based index.
   virtual StoredAdjustment* getAdjustment(uint8_t index) = 0;

   // TopState to return while this mode is active.
   virtual TopState activeState() const = 0;

   // TopState to return when the operator cycles past the last adjustment.
   virtual TopState completedState() const = 0;

   // Short number shown in the credits display to identify this game's settings.
   virtual uint8_t modeId() const = 0;

   // Called once per update() tick before any switch handling.
   // Override for per-tick side effects (e.g. clearing the player-count display).
   virtual void onUpdate() {}

   // Called by exit() — derived classes reload their game's settings from EEPROM.
   virtual void onExit() = 0;

public:
   void setMachine(PinballMachine& m) { machine_ = &m; }

   void     enter(unsigned long currentTime) override;
   void     exit() override;
   TopState update(unsigned long currentTime) override;
};
