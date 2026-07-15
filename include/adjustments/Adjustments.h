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

// 32-bit score stored in EEPROM.  Press adds 1000; double-click resets to 0;
// hold auto-increments with acceleration.  EEPROM writes are deferred during
// hold and flushed on release.
class ScoreAdjustment : public StoredAdjustment {
   int      addr_;
   uint32_t value_            = 0;
   uint32_t nextSpeedyChange_ = 0;
   uint8_t  numSpeedyChanges_ = 0;
   bool     pendingWrite_     = false;

public:
   explicit ScoreAdjustment(int addr) : addr_(addr) {}
   void onEnter(PinballMachine& machine) override;
   void onPress(PinballMachine& machine, bool doubleClick) override;
   void onHeld(PinballMachine& machine, unsigned long currentTime) override;
   void onHeldReleased(PinballMachine& machine) override;
};

// Wrapping integer counter stored in EEPROM.  Each press increments value_
// by STEP; when the result exceeds MAX it wraps back to MIN.  Both single
// press and double-click increment (no reset-to-zero behaviour).
// T may be any integer type supported by EEPROM.get()/put().
template<typename T, T STEP, T MIN, T MAX>
class IntRangeAdjustment : public StoredAdjustment {
   int addr_;
   T   value_ = MIN;

public:
   explicit IntRangeAdjustment(int addr) : addr_(addr) {}
   void onEnter(PinballMachine& machine) override;
   void onPress(PinballMachine& machine, bool doubleClick) override;
};

// Read-only audit counter stored in EEPROM.  Single press is a no-op;
// double-click resets the counter to zero.
// T controls the EEPROM storage width (uint8_t, uint16_t, unsigned long, …).
template<typename T>
class AuditAdjustment : public StoredAdjustment {
   int addr_;
   T   value_ = 0;

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

// ---------------------------------------------------------------------------
// Game-adjustment types (runtime parameters, two-phase init)
// ---------------------------------------------------------------------------

// Byte field that cycles 0 … MAX → MIN.  Call init() once before first use.
class MinMaxByteAdjustment : public StoredAdjustment {
   uint8_t* field_  = nullptr;
   int      eeprom_ = 0;
   uint8_t  min_    = 0;
   uint8_t  max_    = 1;
public:
   void init(uint8_t* field, int eeprom, uint8_t min, uint8_t max);
   void onEnter(PinballMachine& machine) override;
   void onPress(PinballMachine& machine, bool doubleClick) override;
};

// Like MinMaxByteAdjustment but inserts 99 ("use default") between MAX and MIN.
class MinMaxDefaultByteAdjustment : public StoredAdjustment {
   uint8_t* field_  = nullptr;
   int      eeprom_ = 0;
   uint8_t  min_    = 0;
   uint8_t  max_    = 7;
public:
   void init(uint8_t* field, int eeprom, uint8_t min, uint8_t max);
   void onEnter(PinballMachine& machine) override;
   void onPress(PinballMachine& machine, bool doubleClick) override;
};

// Byte field that cycles through a fixed list.  field_ and values_ are
// protected so subclasses can add side effects (e.g. DimLevelAdjustment).
class ListByteAdjustment : public StoredAdjustment {
protected:
   uint8_t*       field_  = nullptr;
   int            eeprom_ = 0;
   const uint8_t* values_ = nullptr;
   uint8_t        count_  = 0;
public:
   void init(uint8_t* field, int eeprom, const uint8_t* values, uint8_t count);
   void onEnter(PinballMachine& machine) override;
   void onPress(PinballMachine& machine, bool doubleClick) override;
};

// 32-bit score value in EEPROM; each press adds 5000 and wraps after 100 000.
class ScoreULAdjustment : public StoredAdjustment {
   unsigned long* field_  = nullptr;
   int            eeprom_ = 0;
public:
   void init(unsigned long* field, int eeprom);
   void onEnter(PinballMachine& machine) override;
   void onPress(PinballMachine& machine, bool doubleClick) override;
};
