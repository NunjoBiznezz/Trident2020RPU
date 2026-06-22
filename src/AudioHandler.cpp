#include "AudioHandler.h"
#include "RPU.h"
#include "RPU_config.h"

AudioHandler::AudioHandler() {
   clearSoundQueue();
}

void AudioHandler::initDevices() {
#if (RPU_OS_HARDWARE_REV >= 2 && defined(RPU_OS_USE_SB300))
   RPU_InitSB300();
   RPU_PlaySB300StartupBeep();
#endif
}

bool AudioHandler::playSound(uint16_t soundIndex, uint8_t audioType, uint8_t overrideVolume) {
   switch (audioType) {
   case AUDIO_PLAY_TYPE_CHIMES:
#if defined(RPU_OS_USE_SB100) && (RPU_OS_HARDWARE_REV == 2)
      RPU_PlaySB100Chime((uint8_t)soundIndex);
      return true;
#endif
      break;

   case AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS:
#ifdef RPU_OS_USE_DASH51
      RPU_PlaySoundDash51((uint8_t)soundIndex);
      return true;
#endif
#ifdef RPU_OS_USE_S_AND_T
      RPU_PlaySoundSAndT((uint8_t)soundIndex);
      return true;
#endif
#ifdef RPU_OS_USE_SB100
      RPU_PlaySB100((uint8_t)soundIndex);
      return true;
#endif
      break;

   default: break;
   }

   (void)soundIndex;
   (void)overrideVolume;
   return false;
}

bool AudioHandler::queueSound(uint16_t soundIndex, uint8_t audioType, unsigned long timeToPlay,
                               uint8_t overrideVolume) {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundQueue[count].playTime == 0) {
         soundQueue[count].soundIndex    = soundIndex;
         soundQueue[count].audioType     = audioType;
         soundQueue[count].playTime      = timeToPlay;
         soundQueue[count].overrideVolume = overrideVolume;
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

void AudioHandler::serviceSoundQueue(unsigned long currentTime) {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundQueue[count].playTime != 0 && soundQueue[count].playTime < currentTime) {
         playSound(soundQueue[count].soundIndex, soundQueue[count].audioType,
                   soundQueue[count].overrideVolume);
         soundQueue[count].playTime = 0;
      }
   }
}

void AudioHandler::update(unsigned long currentTime) {
   serviceSoundQueue(currentTime);
}
