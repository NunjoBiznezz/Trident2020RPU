#pragma once
#include "CircularQueue.h"
#include "RPU_config.h"
#include "TimedStack.h"
#include <stdint.h>

constexpr unsigned RPU_NUM_SOLENOIDS = 15;
constexpr unsigned DEFAULT_SOLENOID_STATE = 0x9F;
constexpr uint8_t SOLENOID_STACK_EMPTY = 0xFF;
constexpr uint8_t SOL_NONE = 0x0F;

namespace RPU {

class SolenoidManager {
public:
   void reset();
   void initDefault();
   void zeroCrossingISR();

   void push(uint8_t solenoidNumber, uint8_t numPushes, bool disableOverride = false);
   void pushToFront(uint8_t solenoidNumber, uint8_t numPushes);
   uint8_t pullFirst();

   bool pushTimed(uint8_t solenoidNumber, uint8_t numPushes, unsigned long whenToFire, bool disableOverride);
   void updateTimed(unsigned long curTime);

   void setCoinLockout(bool lockoutOff, uint8_t solbit);
   void setDisableFlippers(bool disableFlippers, uint8_t solbit);
   void setContinuousBit(bool bitOn, uint8_t solbit);
   bool fireContinuous(uint8_t solBit, uint8_t numCyclesToFire);
   uint8_t readContinuous() const;

   void enable();
   void disable();

private:
   void writeContinuousByte();

#if (RPU_OS_HARDWARE_REV > 2)
   static constexpr uint8_t STACK_CAPACITY = 150;
#else
   static constexpr uint8_t STACK_CAPACITY = 60;
#endif
   CircularQueue<uint8_t, STACK_CAPACITY, SOLENOID_STACK_EMPTY> stack_;
   bool stackEnabled_ = true;
   volatile uint8_t currentByte_ = 0xFF;
   volatile uint8_t revertBit_ = 0x00;
   volatile uint8_t numCyclesBeforeRevert_ = 0;

   static constexpr uint8_t TIMED_STACK_CAPACITY = 30;
   TimedStack<TimedSolenoidEntry, TIMED_STACK_CAPACITY> timedStack_;
};

extern SolenoidManager solenoids;

} // namespace RPU
