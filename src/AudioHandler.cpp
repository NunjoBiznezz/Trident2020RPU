#include "AudioHandler.h"
#include "RPU.h"
#include "RPU_config.h"

AudioHandler::AudioHandler() {
   clearSoundQueue();
}

void AudioHandler::initDevices() {
   RPU_InitNativeAudio();
}

bool AudioHandler::playSound(uint16_t soundIndex) {
      RPU_PlayNativeSound((uint8_t)soundIndex);
      return true;
}

bool AudioHandler::queueSound(uint16_t soundIndex, unsigned long timeToPlay) {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundQueue[count].playTime == 0) {
         soundQueue[count].soundIndex    = soundIndex;
         soundQueue[count].playTime      = timeToPlay;
         return true;
      }
   }
   return false;
}

void AudioHandler::stopAllSoundFX() {
   clearSoundQueue();
}

void AudioHandler::clearSoundQueue() {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      soundQueue[count].playTime = 0;
   }
}

void AudioHandler::update(unsigned long currentTime) {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundQueue[count].playTime != 0 && soundQueue[count].playTime < currentTime) {
         playSound(soundQueue[count].soundIndex);
         soundQueue[count].playTime = 0;
      }
   }
}
