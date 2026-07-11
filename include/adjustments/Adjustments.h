/**************************************************************************
 * adjustments/Adjustments.h
 *
 * StoredAdjustment base class and concrete types used by StoredAdjustmentsMode.
 * PinballMachine is forward-declared so this header has no dependency on the
 * full machine hierarchy.
 **************************************************************************/

#pragma once
#include <stdint.h>

class PinballMachine;

// ---------------------------------------------------------------------------
// StoredAdjustment — base class for a single adjustable setting
// ---------------------------------------------------------------------------

class StoredAdjustment {
public:
   virtual ~StoredAdjustment() = default;

   // Called once when this setting becomes the active one.
   virtual void onEnter(PinballMachine& machine) = 0;

   // Called on SW_CREDIT_RESET press.  doubleClick=true when the press was
   // within 400 ms of the previous one.
   virtual void onPress(PinballMachine& machine, bool doubleClick) {}

   // Called each tick while SW_CREDIT_RESET has been held for > 1300 ms.
   // Implementations that support speedy-increment update their own timing.
   virtual void onHeld(PinballMachine& machine, unsigned long currentTime) {}

   // Called once when SW_CREDIT_RESET is released after a hold period.
   // Implementations that deferred EEPROM writes during hold flush them here.
   virtual void onHeldReleased(PinballMachine& machine) {}

   // Called every tick for per-frame effects (e.g. display blinking).
   virtual void onTick(PinballMachine& machine, unsigned long currentTime) {}

   // Return true if pressing SW_CREDIT_RESET should exit to Attract rather
   // than delegating to onPress.  Used by BootAdjustment.
   virtual bool exitsOnPress() const { return false; }
};

// ---------------------------------------------------------------------------
// Concrete adjustment types
// ---------------------------------------------------------------------------

// Unsigned-long score stored in EEPROM.  Press adds 1000; double-click
// resets to 0; hold auto-increments with acceleration.  EEPROM writes are
// deferred during hold and flushed on release.
class ScoreAdjustment : public StoredAdjustment {
   int           addr_;
   unsigned long value_            = 0;
   unsigned long nextSpeedyChange_ = 0;
   unsigned long numSpeedyChanges_ = 0;
   bool          pendingWrite_     = false;

public:
   explicit ScoreAdjustment(int addr) : addr_(addr) {}
   void onEnter(PinballMachine& machine) override;
   void onPress(PinballMachine& machine, bool doubleClick) override;
   void onHeld(PinballMachine& machine, unsigned long currentTime) override;
   void onHeldReleased(PinballMachine& machine) override;
};

// Byte credit count stored in EEPROM.  Press increments 0–99 and wraps.
// Both single press and double-click increment (no reset-to-zero behaviour).
class CreditsAdjustment : public StoredAdjustment {
   int     addr_;
   uint8_t value_ = 0;

public:
   explicit CreditsAdjustment(int addr) : addr_(addr) {}
   void onEnter(PinballMachine& machine) override;
   void onPress(PinballMachine& machine, bool doubleClick) override;
};

// Read-only audit counter stored in EEPROM.  Single press is a no-op;
// double-click resets the counter to zero.
class AuditAdjustment : public StoredAdjustment {
   int           addr_;
   unsigned long value_ = 0;

public:
   explicit AuditAdjustment(int addr) : addr_(addr) {}
   void onEnter(PinballMachine& machine) override;
   void onPress(PinballMachine& machine, bool doubleClick) override;
};

// Final "boot" entry: displays 8007 on all heads with a blinking mask.
// Any press on SW_CREDIT_RESET exits back to Attract mode.
class BootAdjustment : public StoredAdjustment {
public:
   void onEnter(PinballMachine& machine) override;
   void onTick(PinballMachine& machine, unsigned long currentTime) override;
   bool exitsOnPress() const override { return true; }
};
