#include "AudioHandler.h"
#include "RPU.h"
#include "RPU_config.h"
#include "SoundEffects.h"

#include <Arduino.h>

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

void AudioHandler::playSoundEffect(uint8_t soundEffectNum) {
      unsigned long currentTime_ = millis();

      switch (soundEffectNum) {
      case SOUND_EFFECT_ADD_BONUS:
         this->queueSound(SOUND_NATIVE_ADD_BONUS, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         break;

      case SOUND_EFFECT_POP_BUMPER:
         this->queueSound(SOUND_NATIVE_POP_BUMPER, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         break;

      case SOUND_EFFECT_ONE_HUNDRED:
      case SOUND_EFFECT_ROLLOVER:
      case SOUND_EFFECT_DT_SKILL_SHOT:
      case SOUND_EFFECT_ROLLOVER_SKILL_SHOT:
      case SOUND_EFFECT_SU_SKILL_SHOT:
      case SOUND_EFFECT_LEFT_SPINNER:
      case SOUND_EFFECT_RIGHT_SPINNER:
      case SOUND_EFFECT_DROP_TARGET:
      case SOUND_EFFECT_BALL_OVER:
         this->queueSound(SOUND_NATIVE_ONE_HUNDRED, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         break;

      case SOUND_EFFECT_LEFT_INLANE:
         this->queueSound(SOUND_NATIVE_ONE_HUNDRED, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         // for (unsigned count = 0; count < optionalValue; count++) {
         //    this->queueSound(SOUND_NATIVE_ONE_HUNDRED, currentTime_ + 200 * count);
         //    this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75 + (200 * count));
         // }
         break;

         case SOUND_EFFECT_RIGHT_INLANE:
         // Repeat 100 3 times, then 1000 3 times with 125ms between each sound
         for (int count = 0; count < 6; count++) {
            this->queueSound((count < 3) ? SOUND_NATIVE_ONE_HUNDRED : SOUND_NATIVE_ONE_THOUSAND, currentTime_ + 200 * count);
            this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_5K:
         // Repeat 100 5 times with 125ms between each sound
         for (int count = 0; count < 5; count++) {
            this->queueSound(SOUND_NATIVE_ONE_HUNDRED, currentTime_ + 200 * count);
            this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_30K:
         // Repeat 10,000 3 times with 125ms between each sound
         for (int count = 0; count < 3; count++) {
            this->queueSound(SOUND_NATIVE_TEN_THOUSAND, currentTime_ + 200 * count);
            this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_20K:
         // Repeat 10,000 2 times with 125ms between each sound
         for (int count = 0; count < 2; count++) {
            this->queueSound(SOUND_NATIVE_TEN_THOUSAND, currentTime_ + 200 * count);
            this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_10K:
         // Repeat 10,000 1 times with 125ms between each sound
         for (int count = 0; count < 1; count++) { // WTF?
            this->queueSound(SOUND_NATIVE_TEN_THOUSAND, currentTime_ + 200 * count);
            this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_RIGHT_OUTLANE:
         // Repeat 1,000 5 times with 125ms between each sound
         for (int count = 0; count < 5; count++) {
            this->queueSound(SOUND_NATIVE_ONE_THOUSAND, currentTime_ + 200 * count);
            this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75 + (200 * count));
         }
         break;

      case SOUND_EFFECT_ONE_THOUSAND:
      case SOUND_EFFECT_TOP_BUMPER_HIT:
      case SOUND_EFFECT_BOTTOM_BUMPER_HIT:
         this->queueSound(SOUND_NATIVE_ONE_THOUSAND, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         break;

      case SOUND_EFFECT_SHOOT_AGAIN:
      case SOUND_EFFECT_PLAYER_1_UP:
      case SOUND_EFFECT_PLAYER_2_UP:
      case SOUND_EFFECT_PLAYER_3_UP:
      case SOUND_EFFECT_PLAYER_4_UP:
         this->queueSound(SOUND_NATIVE_ONE_THOUSAND, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 175);
         break;

      case SOUND_EFFECT_BONUS_COUNT:
      case SOUND_EFFECT_2X_BONUS_COUNT:
      case SOUND_EFFECT_3X_BONUS_COUNT:
      case SOUND_EFFECT_4X_BONUS_COUNT:
      case SOUND_EFFECT_5X_BONUS_COUNT:
         this->queueSound(SOUND_NATIVE_ONE_THOUSAND, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         break;

      case SOUND_EFFECT_UPPER_SLING:
      case SOUND_EFFECT_EXTRA_BALL:
      case SOUND_EFFECT_TILT_WARNING:
         this->queueSound(SOUND_NATIVE_ONE_THOUSAND, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         break;

      case SOUND_EFFECT_TEN:
      case SOUND_EFFECT_10PT_SWITCH:
      case SOUND_EFFECT_MATCH_SPIN:
      case SOUND_EFFECT_LOWER_SLING:
         this->queueSound(SOUND_NATIVE_TEN, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         break;

      case SOUND_EFFECT_TEN_THOUSAND:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_1:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_2:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_3:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_4:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_5:
         this->queueSound(SOUND_NATIVE_TEN_THOUSAND, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         break;

      case SOUND_EFFECT_FIRST_SU_SWITCH_HIT:
      case SOUND_EFFECT_SECOND_SU_SWITCH_HIT:
      case SOUND_EFFECT_THIRD_SU_SWITCH_HIT:
      case SOUND_EFFECT_FOURTH_SU_SWITCH_HIT:
      case SOUND_EFFECT_FIFTH_SU_SWITCH_HIT:
         this->queueSound(SOUND_NATIVE_ONE_THOUSAND, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         break;

      case SOUND_EFFECT_ADD_CREDIT:
      case SOUND_EFFECT_GAME_OVER:
         this->queueSound(SOUND_NATIVE_TEN_THOUSAND, currentTime_);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 75);
         this->queueSound(SOUND_NATIVE_ONE_HUNDRED, currentTime_ + 150);
         this->queueSound(SOUND_NATIVE_TEN, currentTime_ + 225);
         this->queueSound(SOUND_NATIVE_TEN_THOUSAND, currentTime_ + 325);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 400);
         this->queueSound(SOUND_NATIVE_ONE_HUNDRED, currentTime_ + 475);
         this->queueSound(SOUND_NATIVE_TEN, currentTime_ + 550);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 650);
         break;

      case SOUND_EFFECT_ADD_PLAYER_1:
      case SOUND_EFFECT_ADD_PLAYER_2:
      case SOUND_EFFECT_ADD_PLAYER_3:
      case SOUND_EFFECT_ADD_PLAYER_4:
      case SOUND_EFFECT_RESCUE_FROM_THE_DEEP:
      case SOUND_EFFECT_TRIDENT_INTRO:
         this->queueSound(SOUND_NATIVE_TEN, currentTime_);
         this->queueSound(SOUND_NATIVE_ONE_HUNDRED, currentTime_ + 75);
         this->queueSound(SOUND_NATIVE_ONE_THOUSAND, currentTime_ + 150);
         this->queueSound(SOUND_NATIVE_TEN_THOUSAND, currentTime_ + 225);
         this->queueSound(SOUND_NATIVE_TEN, currentTime_ + 325);
         this->queueSound(SOUND_NATIVE_ONE_HUNDRED, currentTime_ + 400);
         this->queueSound(SOUND_NATIVE_ONE_THOUSAND, currentTime_ + 475);
         this->queueSound(SOUND_NATIVE_TEN_THOUSAND, currentTime_ + 550);
         this->queueSound(SOUND_NATIVE_NONE, currentTime_ + 650);
         break;
      }
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

void AudioHandler::update(unsigned long currentTime) {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundQueue[count].playTime != 0 && soundQueue[count].playTime < currentTime) {
         playSound(soundQueue[count].soundIndex);
         soundQueue[count].playTime = 0;
      }
   }
}
