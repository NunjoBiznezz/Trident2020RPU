#include "AudioHandler.h"
#include "RPU.h"
#include "RPU_config.h"

AudioHandler::AudioHandler() {
   clearSoundQueue();
}

void AudioHandler::initDevices() {
   RPU_InitNativeAudio();
}

bool AudioHandler::playSound(uint16_t soundIndex, uint8_t audioType) {
   switch (audioType) {
   case AUDIO_PLAY_TYPE_CHIMES:
#if defined(RPU_OS_USE_SB100) && (RPU_OS_HARDWARE_REV == 2)
      RPU_PlayNativeChime((uint8_t)soundIndex);
      return true;
#endif
   case AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS:
      RPU_PlayNativeSound((uint8_t)soundIndex);
      return true;
   default: break;
   }
   return false;
}

bool AudioHandler::queueSound(uint16_t soundIndex, unsigned long timeToPlay, uint8_t audioType) {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundQueue[count].playTime == 0) {
         soundQueue[count].soundIndex    = soundIndex;
         soundQueue[count].playTime      = timeToPlay;
         soundQueue[count].audioType     = audioType;
         return true;
      }
   }
   return false;
}

void AudioHandler::stopAllSoundFX() {
   clearSoundQueue();
#if defined(RPU_OS_USE_SB100) || defined(RPU_OS_USE_DASH51) || defined(RPU_OS_USE_S_AND_T)
   RPU_PlayNativeSound(0);
#endif
}

void AudioHandler::clearSoundQueue() {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      soundQueue[count].playTime = 0;
   }
}

void AudioHandler::serviceSoundQueue(unsigned long currentTime) {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundQueue[count].playTime != 0 && soundQueue[count].playTime < currentTime) {
         playSound(soundQueue[count].soundIndex, soundQueue[count].audioType);
         soundQueue[count].playTime = 0;
      }
   }
}

void AudioHandler::update(unsigned long currentTime) {
   serviceSoundQueue(currentTime);
}
