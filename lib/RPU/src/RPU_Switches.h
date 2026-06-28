#pragma once
#include "CircularQueue.h"
#include "RPU.h"
#include "RPU_config.h"
#include <stdint.h>

constexpr unsigned NUM_SWITCH_BYTES = 5;
constexpr unsigned NUM_SWITCH_BYTES_ON_U10_PORT_A = 5;
constexpr unsigned MAX_NUM_SWITCHES = 40;

constexpr int RPU_OS_SWITCH_DELAY_IN_MICROSECONDS = 200;
constexpr int RPU_OS_TIMING_LOOP_PADDING_IN_MICROSECONDS = 70;

class SwitchManager {
public:
   void reset();
   void service();

   void pushToStack(uint8_t switchNumber);
   void pushSelfTest();
   uint8_t pullFromStack();
   bool readState(uint8_t switchNum) const;

   void setup(int numSwitches, int numPrioritySwitches, const PlayfieldAndCabinetSwitch* switchArray);
   void clearUpDown();
   bool getUpDown() const;

private:
   int numGameSwitches_ = 0;
   int numPrioritySwitches_ = 0;
   const PlayfieldAndCabinetSwitch* gameSwitches_ = nullptr;

   volatile uint8_t minus2_[NUM_SWITCH_BYTES] = {};
   volatile uint8_t minus1_[NUM_SWITCH_BYTES] = {};
   volatile uint8_t now_[NUM_SWITCH_BYTES] = {};

   CircularQueue<uint8_t, 60, 0xFF> switchStack_;

   void serviceBank(uint8_t switchCount);
};

extern SwitchManager switches;
