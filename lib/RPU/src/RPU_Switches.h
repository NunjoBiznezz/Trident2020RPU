#pragma once
#include "CircularQueue.h"
#include "RPU.h"
#include "RPU_config.h"
#include <stdint.h>

// Forward declarations of internal RPU bus functions defined in RPU.cpp
void RPU_DataWrite(int address, uint8_t data);
uint8_t RPU_DataRead(int address);

constexpr unsigned NUM_SWITCH_BYTES = 5;
constexpr unsigned NUM_SWITCH_BYTES_ON_U10_PORT_A = 5;
constexpr unsigned MAX_NUM_SWITCHES = 40;

class SwitchManager {
public:
   void reset();
   void service();

   void pushToStack(uint8_t switchNumber);
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
