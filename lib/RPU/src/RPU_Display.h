#pragma once
#include "RPU_config.h"
#include "RPU.h"
#include <stdint.h>

class DisplayManager {
public:
   void reset();
   void serviceISR();

   uint8_t setDisplay(int displayNumber, unsigned long value, bool blankByMagnitude = false, uint8_t minDigits = 0,
                      bool showCommasByMagnitude = false);
   void setCredits(int value, bool displayOn, bool showBothDigits = false);
   void setBallInPlay(int value, bool displayOn, bool showBothDigits = false);
   void cycleAll(unsigned long curTime, uint8_t digitNum = 0);
   void setMatch(int value, bool displayOn, bool showBothDigits = false);
   void setBlank(int displayNumber, uint8_t bitMask);
   uint8_t getBlank(int displayNumber) const;
   void setFlash(int displayNumber, unsigned long value, unsigned long curTime, unsigned period, uint8_t minDigits = 0);
   void setFlashCredits(unsigned long curTime, int period);

private:
   volatile uint8_t digits_[5][RPU_OS_NUM_DIGITS] = {};
   volatile uint8_t digitEnable_[5] = {};
   volatile bool offCycle_ = false;
   volatile uint8_t currentDigit_ = 0;
};

extern DisplayManager displays;
