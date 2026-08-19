#include "RPU_config.h"

#if defined(RPU_OS_USE_WTYPE_1_SOUND) || defined(RPU_OS_USE_WTYPE_2_SOUND)

#include <stdint.h>
#include "RPU.h"
#include "RPU_Internal.h"

namespace RPU {
#if defined(RPU_OS_USE_WTYPE_2_SOUND)
unsigned short SoundLowerLimit = 0x0000;
unsigned short SoundUpperLimit = 0x0080;
#else
unsigned short SoundLowerLimit = 0x0100;
unsigned short SoundUpperLimit = 0x1F00;
#endif

#define SOUND_STACK_SIZE  64
#define SOUND_STACK_EMPTY 0x0000
volatile uint8_t SoundStackFirst;
volatile uint8_t SoundStackLast;
volatile uint16_t SoundStack[SOUND_STACK_SIZE];

#define TIMED_SOUND_STACK_SIZE  20
struct TimedSoundEntry {
   uint8_t inUse;
   unsigned long pushTime;
   unsigned short soundNumber;
   uint8_t numPushes;
};
TimedSoundEntry TimedSoundStack[TIMED_SOUND_STACK_SIZE] = {0, 0, 0, 0};

void RPU_SetSoundValueLimits(unsigned short lowerLimit, unsigned short upperLimit) {
   SoundLowerLimit = lowerLimit;
   SoundUpperLimit = upperLimit;
}

int SpaceLeftOnSoundStack() {
   if (SoundStackFirst >= SOUND_STACK_SIZE || SoundStackLast >= SOUND_STACK_SIZE) {
      return 0;
   }
   if (SoundStackLast >= SoundStackFirst) {
      return ((SOUND_STACK_SIZE - 1) - (SoundStackLast - SoundStackFirst));
   }
   return (SoundStackFirst - SoundStackLast) - 1;
}

void RPU_PushToSoundStack(unsigned short soundNumber, uint8_t numPushes) {
   // If the solenoid stack last index is out of range, then it's an error - return
   if (SpaceLeftOnSoundStack() == 0) {
      return;
   }
   if (soundNumber < SoundLowerLimit || soundNumber > SoundUpperLimit) {
      return;
   }

   for (int count = 0; count < numPushes; count++) {
      SoundStack[SoundStackLast] = soundNumber;

      SoundStackLast += 1;
      if (SoundStackLast == SOUND_STACK_SIZE) {
         // If the end index is off the end, then wrap
         SoundStackLast = 0;
      }
      // If the stack is now full, return
      if (SpaceLeftOnSoundStack() == 0) {
         return;
      }
   }
}

unsigned short PullFirstFromSoundStack() {
   // If first and last are equal, there's nothing on the stack
   if (SoundStackFirst == SoundStackLast) {
      return SOUND_STACK_EMPTY;
   }

   unsigned short retVal = SoundStack[SoundStackFirst];

   SoundStackFirst += 1;
   if (SoundStackFirst >= SOUND_STACK_SIZE) {
      SoundStackFirst = 0;
   }

   return retVal;
}

bool RPU_PushToTimedSoundStack(unsigned short soundNumber, uint8_t numPushes, unsigned long whenToPlay) {
   uint8_t slot = TimedSoundStack.findFreeSlot();
   if (slot >= TimedSoundStack.getSize()) {
      return false;
   }
   TimedSoundStack.entries[slot].inUse = true;
   TimedSoundStack.entries[slot].pushTime = whenToPlay;
   TimedSoundStack.entries[slot].soundNumber = soundNumber;
   TimedSoundStack.entries[slot].numPushes = numPushes;
   return true;
}

void RPU_UpdateTimedSoundStack(unsigned long curTime) {
   for (int count = 0; count < TimedSoundStack.getSize(); count++) {
      if (TimedSoundStack.entries[count].inUse && TimedSoundStack.entries[count].pushTime < curTime) {
         RPU_PushToSoundStack(TimedSoundStack.entries[count].soundNumber, TimedSoundStack.entries[count].numPushes);
         TimedSoundStack.entries[count].inUse = false;
      }
   }
}

} // namespace RPU

#endif
