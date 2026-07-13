/**************************************************************************
 * adjustments/Adjustments.cpp
 **************************************************************************/

#include "adjustments/Adjustments.h"
#include "PinballMachine.h"
#include "Trident2020.h"
#include <EEPROM.h>

static unsigned long readUL(int addr) {
   unsigned long val = 0;
   EEPROM.get(addr, val);
   return val;
}

static void writeUL(int addr, unsigned long val) {
   EEPROM.put(addr, val);
}

// ---------------------------------------------------------------------------
// ScoreAdjustment
// ---------------------------------------------------------------------------

void ScoreAdjustment::onEnter(PinballMachine& machine) {
   value_            = readUL(addr_);
   nextSpeedyChange_ = 0;
   numSpeedyChanges_ = 0;
   pendingWrite_     = false;
   machine.setDisplay(0, value_, true);
}

void ScoreAdjustment::onPress(PinballMachine& machine, bool doubleClick) {
   if (doubleClick) {
      value_ = 0;
   } else {
      value_ += 1000;
   }
   machine.setDisplay(0, value_, true);
   writeUL(addr_, value_);
   nextSpeedyChange_ = 0;
   numSpeedyChanges_ = 0;
   pendingWrite_     = false;
}

void ScoreAdjustment::onHeld(PinballMachine& machine, unsigned long currentTime) {
   if (nextSpeedyChange_ == 0) {
      nextSpeedyChange_ = currentTime;
      numSpeedyChanges_ = 0;
   }
   if (currentTime >= nextSpeedyChange_) {
      value_ += 1000;
      machine.setDisplay(0, value_, true);
      numSpeedyChanges_++;
      if (numSpeedyChanges_ < 6) {
         nextSpeedyChange_ = currentTime + 400;
      } else if (numSpeedyChanges_ < 50) {
         nextSpeedyChange_ = currentTime + 50;
      } else {
         nextSpeedyChange_ = currentTime + 10;
      }
      pendingWrite_ = true;
   }
}

void ScoreAdjustment::onHeldReleased(PinballMachine& /*machine*/) {
   if (pendingWrite_) {
      writeUL(addr_, value_);
      pendingWrite_ = false;
   }
   nextSpeedyChange_ = 0;
   numSpeedyChanges_ = 0;
}

// ---------------------------------------------------------------------------
// IntRangeAdjustment
// ---------------------------------------------------------------------------

template<typename T, T STEP, T MIN, T MAX>
void IntRangeAdjustment<T, STEP, MIN, MAX>::onEnter(PinballMachine& machine) {
   EEPROM.get(addr_, value_);
   machine.setDisplay(0, (unsigned long)value_, true);
}

template<typename T, T STEP, T MIN, T MAX>
void IntRangeAdjustment<T, STEP, MIN, MAX>::onPress(PinballMachine& machine, bool /*doubleClick*/) {
   value_ += STEP;
   if (value_ > MAX) {
      value_ = MIN;
   }
   machine.setDisplay(0, (unsigned long)value_, true);
   EEPROM.put(addr_, value_);
}

template class IntRangeAdjustment<uint8_t, 1, 0, 99>;

// ---------------------------------------------------------------------------
// AuditAdjustment
// ---------------------------------------------------------------------------

void AuditAdjustment::onEnter(PinballMachine& machine) {
   value_ = readUL(addr_);
   machine.setDisplay(0, value_, true);
}

void AuditAdjustment::onPress(PinballMachine& machine, bool doubleClick) {
   if (!doubleClick) {
      return;
   }
   value_ = 0;
   machine.setDisplay(0, value_, true);
   writeUL(addr_, value_);
}

// ---------------------------------------------------------------------------
// BootAdjustment
// ---------------------------------------------------------------------------

void BootAdjustment::onEnter(PinballMachine& machine) {
   for (int i = 0; i < 4; i++) {
      machine.setDisplay(i, 8007, true);
   }
}

void BootAdjustment::onTick(PinballMachine& machine, unsigned long currentTime) {
   for (int i = 0; i < 4; i++) {
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS
      machine.setDisplayBlank(i, ((currentTime / 500) % 2) ? 0x78 : 0x00);
#else
      machine.setDisplayBlank(i, ((currentTime / 500) % 2) ? 0x3C : 0x00);
#endif
   }
}
