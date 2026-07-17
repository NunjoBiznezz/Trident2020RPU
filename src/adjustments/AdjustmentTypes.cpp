/**************************************************************************
 * adjustments/Adjustments.cpp
 **************************************************************************/

#include "adjustments/AdjustmentTypes.h"
#include "PinballMachine.h"
#include "Trident2020.h"
#include <EEPROM.h>

// ---------------------------------------------------------------------------
// ScoreAdjustment
// ---------------------------------------------------------------------------

void ScoreAdjustment::init(unsigned long* field, int addr) {
   field_ = field;
   addr_  = addr;
}

void ScoreAdjustment::onEnter(PinballMachine& machine) {
   value_            = *field_;
   nextSpeedyChange_ = 0;
   numSpeedyChanges_ = 0;
   pendingWrite_     = false;
   machine.setDisplay(0, value_, true);
}

void ScoreAdjustment::onPress(PinballMachine& machine, bool doubleClick) {
   value_            = doubleClick ? 0 : value_ + 1000;
   *field_           = value_;
   nextSpeedyChange_ = 0;
   numSpeedyChanges_ = 0;
   pendingWrite_     = false;
   machine.setDisplay(0, value_, true);
   EEPROM.put(addr_, value_);
}

void ScoreAdjustment::onHeld(PinballMachine& machine, unsigned long currentTime) {
   if (nextSpeedyChange_ == 0) {
      nextSpeedyChange_ = currentTime;
      numSpeedyChanges_ = 0;
   }
   if (currentTime >= nextSpeedyChange_) {
      value_ += 1000;
      *field_ = value_;
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
      EEPROM.put(addr_, value_);
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

template<typename T>
void AuditAdjustment<T>::init(T* field, int eeprom) {
   field_ = field;
   eeprom_ = eeprom;
}

template<typename T>
void AuditAdjustment<T>::onEnter(PinballMachine& machine) {
   machine.setDisplay(0, (unsigned long)*field_, true);
}

template<typename T>
void AuditAdjustment<T>::onPress(PinballMachine& machine, bool doubleClick) {
   if (!doubleClick) {
      return;
   }
   *field_ = 0;
   machine.setDisplay(0, 0, true);
   EEPROM.put(eeprom_, *field_);
}

template class AuditAdjustment<uint32_t>;

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

// ---------------------------------------------------------------------------
// MinMaxByteAdjustment
// ---------------------------------------------------------------------------

void MinMaxByteAdjustment::init(uint8_t* field, int eeprom, uint8_t min, uint8_t max) {
   field_ = field;
   eeprom_ = eeprom;
   min_ = min;
   max_ = max;
}

void MinMaxByteAdjustment::onEnter(PinballMachine& machine) {
   machine.setDisplay(0, (unsigned long)*field_, true);
}

void MinMaxByteAdjustment::onPress(PinballMachine& machine, bool /*doubleClick*/) {
   uint8_t val = *field_ + 1;
   if (val > max_) val = min_;
   *field_ = val;
   EEPROM.write(eeprom_, val);
   machine.setDisplay(0, (unsigned long)val, true);
}

// ---------------------------------------------------------------------------
// MinMaxDefaultByteAdjustment
// ---------------------------------------------------------------------------

void MinMaxDefaultByteAdjustment::init(uint8_t* field, int eeprom, uint8_t min, uint8_t max) {
   field_ = field;
   eeprom_ = eeprom;
   min_ = min;
   max_ = max;
}

void MinMaxDefaultByteAdjustment::onEnter(PinballMachine& machine) {
   machine.setDisplay(0, (unsigned long)*field_, true);
}

void MinMaxDefaultByteAdjustment::onPress(PinballMachine& machine, bool /*doubleClick*/) {
   uint8_t val = *field_ + 1;
   if (val > max_) {
      val = (val > 99) ? min_ : 99;
   }
   *field_ = val;
   EEPROM.write(eeprom_, val);
   machine.setDisplay(0, (unsigned long)val, true);
}

// ---------------------------------------------------------------------------
// ListByteAdjustment
// ---------------------------------------------------------------------------

void ListByteAdjustment::init(uint8_t* field, int eeprom, const uint8_t* values, uint8_t count) {
   field_ = field;
   eeprom_ = eeprom;
   values_ = values;
   count_ = count;
}

void ListByteAdjustment::onEnter(PinballMachine& machine) {
   machine.setDisplay(0, (unsigned long)*field_, true);
}

void ListByteAdjustment::onPress(PinballMachine& machine, bool /*doubleClick*/) {
   uint8_t cur      = *field_;
   uint8_t newIndex = 0;
   for (uint8_t i = 0; i < count_ - 1; i++) {
      if (cur == values_[i]) newIndex = i + 1;
   }
   *field_ = values_[newIndex];
   EEPROM.write(eeprom_, values_[newIndex]);
   machine.setDisplay(0, (unsigned long)values_[newIndex], true);
}

// ---------------------------------------------------------------------------
// ScoreULAdjustment
// ---------------------------------------------------------------------------

void ScoreULAdjustment::init(unsigned long* field, int eeprom) {
   field_ = field;
   eeprom_ = eeprom;
}

void ScoreULAdjustment::onEnter(PinballMachine& machine) {
   machine.setDisplay(0, *field_, true);
}

void ScoreULAdjustment::onPress(PinballMachine& machine, bool /*doubleClick*/) {
   unsigned long val = *field_ + 5000;
   if (val > 100000) val = 0;
   *field_ = val;
   EEPROM.put(eeprom_, val);
   machine.setDisplay(0, val, true);
}
