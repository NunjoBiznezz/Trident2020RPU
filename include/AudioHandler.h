#pragma once

#include <stdint.h>

/*********************************************************************
    Native SB100 Effect Definitions
*********************************************************************/
constexpr uint8_t SOUND_NATIVE_NONE = 0x00;           // This turns sounds off
constexpr uint8_t SOUND_NATIVE_TEN = 0x01;            // This plays the ten sound
constexpr uint8_t SOUND_NATIVE_ONE_HUNDRED = 0x02;    // This plays the one hundred sound
constexpr uint8_t SOUND_NATIVE_ONE_THOUSAND = 0x04;   // This plays the one thousand sound
constexpr uint8_t SOUND_NATIVE_TEN_THOUSAND = 0x08;   // This plays the ten thousand sound
constexpr uint8_t SOUND_NATIVE_ADD_BONUS = 0x10;      // This plays the add bonus sound
constexpr uint8_t SOUND_NATIVE_POP_BUMPER = 0x20;     // This plays the pop bumper sound

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
