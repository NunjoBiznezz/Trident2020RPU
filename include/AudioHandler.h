#pragma once

#include <stdint.h>

// These are also used as masks in initDevices, so leave them as powers of 2.
constexpr uint8_t AUDIO_PLAY_TYPE_CHIMES          = 1;
constexpr uint8_t AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS = 2;

struct SoundEntry {
   uint16_t soundIndex;
   uint8_t  audioType;
   uint8_t  overrideVolume;
   unsigned long playTime;
};

// Manages native sound card output (SB-100, SB-300, Dash-51, S&T).
// WAV Trigger audio is handled by WavTriggerHandler in main.cpp.
class AudioHandler {
public:
   AudioHandler();

   void initDevices();

   bool playSound(uint16_t soundIndex, uint8_t audioType, uint8_t overrideVolume = 0xFF);

   bool queueSound(uint16_t soundIndex, uint8_t audioType, unsigned long timeToPlay,
                   uint8_t overrideVolume = 0xFF);
   inline bool queueOriginalSound(uint16_t soundIndex, unsigned long timeToPlay,
                                   uint8_t overrideVolume = 0xFF) {
      return queueSound(soundIndex, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, timeToPlay, overrideVolume);
   }

   void stopAllSoundFX();
   void update(unsigned long currentTime);

private:
   static constexpr int SOUND_QUEUE_SIZE = 30;

   SoundEntry soundQueue[SOUND_QUEUE_SIZE];

   void clearSoundQueue();
   void serviceSoundQueue(unsigned long currentTime);
};
