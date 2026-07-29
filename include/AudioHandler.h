#pragma once

#include <stdint.h>

struct SoundEntry {
   uint16_t soundIndex;
   unsigned long playTime;
};

// Manages native sound card output (SB-100, SB-300, Dash-51, S&T).
// WAV Trigger audio is handled by WavTriggerHandler in main.cpp.
class AudioHandler {
public:
   AudioHandler();

   void initDevices();

   bool playSound(uint16_t soundIndex);
   bool queueSound(uint16_t soundIndex, unsigned long timeToPlay);

   void stopAllSoundFX();
   void update(unsigned long currentTime);

private:
   static constexpr int SOUND_QUEUE_SIZE = 30;

   SoundEntry soundQueue[SOUND_QUEUE_SIZE];

   void clearSoundQueue();
};
