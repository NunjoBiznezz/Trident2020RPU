#pragma once
#include "RPU_config.h"
#include <stdint.h>

// Forward declaration of internal RPU bus function defined in RPU.cpp
void RPU_DataWrite(int address, uint8_t data);
uint8_t RPU_DataRead(int address);

class LampManager {
public:
   void reset();
   void strobe();

   void setDimDivisor(uint8_t level, uint8_t divisor);
   void setState(int lampNum, bool lampState, uint8_t lampDim = 0, int lampFlashPeriod = 0);
   uint8_t readState(int lampNum) const;
   uint8_t readDim(int lampNum) const;
   int readFlash(int lampNum) const;

   void applyFlash(unsigned long curTime);
   void flashAll(unsigned long curTime);
   void turnOffAll();

private:
   static const uint8_t BIT_SHIFT[8];

   volatile uint8_t states_[RPU_NUM_LAMP_BANKS] = {};
   volatile uint8_t dim1_[RPU_NUM_LAMP_BANKS] = {};
   volatile uint8_t dim2_[RPU_NUM_LAMP_BANKS] = {};
   volatile uint8_t flashPeriod_[RPU_MAX_LAMPS] = {};
   uint8_t dimDivisor1_ = 2;
   uint8_t dimDivisor2_ = 3;
   volatile int numInterrupts_ = 0;
};

extern LampManager lamps;
