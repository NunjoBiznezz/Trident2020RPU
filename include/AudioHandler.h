#ifndef AUDIO_HANDLER_H

#include "NotificationQueue.h"
#include "RPU.h"
#include "RPU_config.h"
#include <HardwareSerial.h>
#include <stdint.h>

#if defined(RPU_OS_USE_WAV_TRIGGER)
class WavTrigger;
#endif

// These are also used as masks in the InitDevices code, so leave them as powers of 2
constexpr uint8_t AUDIO_PLAY_TYPE_CHIMES = 1; // Not used?
constexpr uint8_t AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS = 2;
constexpr uint8_t AUDIO_PLAY_TYPE_WAV_TRIGGER = 4;

struct AudioSoundtrack {
   uint16_t TrackIndex;
   uint16_t TrackLength;
};


#if defined(RPU_OS_USE_SB300)
const uint8_t SB300_SOUND_FUNCTION_SQUARE_WAVE = 0;
const uint8_t SB300_SOUND_FUNCTION_ANALOG = 1;

struct SoundCardCommandEntry {
   uint8_t soundFunction;
   uint8_t soundRegister;
   uint8_t soundByte;
   unsigned long playTime;
};
#endif

struct SoundEntry {
   uint16_t soundIndex;
   uint8_t audioType;
   uint8_t overrideVolume;
   unsigned long playTime;
};

class AudioHandler {
 public:
   AudioHandler();
   virtual ~AudioHandler();

   bool initDevices(uint8_t audioType);

   void outputTracksPlaying();

   void setSoundFXVolume(uint8_t s_volume);
   void setNotificationsVolume(uint8_t s_volume);
   void setMusicVolume(uint8_t s_volume);
   void setMusicDuckingGain(uint8_t s_ducking);
   void setSoundFXDuckingGain(uint8_t s_ducking);

   bool playBackgroundSoundtrack(AudioSoundtrack* soundtrackArray, uint16_t numSoundtrackEntries, unsigned long currentTime,
                                 bool randomOrder = true);
   bool playBackgroundSong(uint16_t trackIndex, bool loopTrack = true);

   bool playSound(uint16_t soundIndex, uint8_t audioType, uint8_t overrideVolume = 0xFF);
   bool fadeSound(uint16_t soundIndex, int fadeGain, int numMilliseconds, bool stopTrack);

   bool queueSound(uint16_t soundIndex, uint8_t audioType, unsigned long timeToPlay, uint8_t overrideVolume = 0xFF);
   inline bool queueOriginalSound(uint16_t soundIndex, unsigned long timeToPlay, uint8_t overrideVolume = 0xFF) {
      return queueSound(soundIndex, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, timeToPlay, overrideVolume);
   }
   inline bool queueWavTriggerSound(uint16_t soundIndex, unsigned long timeToPlay, uint8_t overrideVolume = 0xFF) {
      return queueSound(soundIndex, AUDIO_PLAY_TYPE_WAV_TRIGGER, timeToPlay, overrideVolume);
   }

   // Not used in this code
   bool queueSoundCardCommand(uint8_t scFunction, uint8_t scRegister, uint8_t scData, unsigned long startTime);


   bool queuePrioritizedNotification(uint16_t notificationIndex, uint16_t notificationLength, uint8_t priority, unsigned long currentTime);

   bool update(unsigned long currentTime);

   bool stopSound(uint16_t soundIndex);
   bool stopCurrentNotification(uint8_t priority = 10);
   bool stopAllMusic();
   bool stopAllNotifications(uint8_t priority = 10);
   bool stopAllSoundFX();
   bool stopAllAudio();

 private:
   static constexpr int NUMBER_OF_SONGS_REMEMBERED = 10;
   static constexpr int VOICE_NOTIFICATION_STACK_SIZE = 5;
   static constexpr int SOUND_QUEUE_SIZE = 30;
   static constexpr int SOUND_EFFECT_QUEUE_SIZE = 50;

   AudioSoundtrack* curSoundtrack;
   int soundFXGain;
   int notificationsGain;
   int musicGain;
   int musicDucking;
   int soundFXDucking;

   NotificationQueue<VOICE_NOTIFICATION_STACK_SIZE> notificationQueue_;
   uint8_t currentNotificationPriority;
   unsigned int currentNotificationPlaying;
   unsigned long currentNotificationStartTime;

   bool soundtrackRandomOrder;
   unsigned int lastSongsPlayed[NUMBER_OF_SONGS_REMEMBERED];
   unsigned long nextSoundtrackPlayTime;
   uint16_t curSoundtrackEntries;
   uint16_t currentBackgroundTrack;

   SoundEntry soundQueue[SOUND_QUEUE_SIZE];

   unsigned long nextVoiceNotificationPlayTime;
   unsigned long backgroundSongEndTime;

#if defined(RPU_OS_USE_SB300)
   static constexpr int SOUND_CARD_QUEUE_SIZE = 100;
   SoundCardCommandEntry soundCardQueue[SOUND_CARD_QUEUE_SIZE];
#endif

#if defined(RPU_OS_USE_WAV_TRIGGER)
   WavTrigger* wTrig = nullptr; // Our WAV Trigger object
#endif

   int convertVolumeSettingToGain(uint8_t volumeSetting);

#ifdef RPU_OS_USE_SB300
   void initSB300Registers();
   void playSB300StartupBeep();
#endif
   void duckCurrentSoundEffects();

   void clearSoundQueue();
   void clearSoundCardQueue();
   void startNextSoundtrackSong(unsigned long currentTime);
   void manageBackgroundSong(unsigned long currentTime);

   bool serviceNotificationQueue(unsigned long currentTime);

   bool serviceSoundCardQueue(unsigned long currentTime);
   bool serviceSoundQueue(unsigned long currentTime);
};

#define AUDIO_HANDLER_H
#endif
