#ifndef AUDIO_HANDLER_H

#include "RPU.h"
#include "RPU_config.h"
#include "WavTrigger.h"
#include <HardwareSerial.h>
#include <stdint.h>

#if defined(RPU_OS_USE_WAV_TRIGGER_1p3)
#define AUDIOHANDLER_USES_WAV_TRIGGER
#define AUDIOHANDLER_USES_WAV_TRIGGER_1P3
#elif defined(RPU_OS_USE_WAV_TRIGGER)
#define AUDIOHANDLER_USES_WAV_TRIGGER
#endif

// These are also used as masks in the InitDevices code, so leave them as powers of 2
constexpr uint8_t AUDIO_PLAY_TYPE_CHIMES = 1; // Not used?
constexpr uint8_t AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS = 2;
constexpr uint8_t AUDIO_PLAY_TYPE_WAV_TRIGGER = 4;

struct AudioSoundtrack {
   uint16_t TrackIndex;
   uint16_t TrackLength;
};

#define SB300_SOUND_FUNCTION_SQUARE_WAVE 0
#define SB300_SOUND_FUNCTION_ANALOG 1

struct SoundCardCommandEntry {
   uint8_t soundFunction;
   uint8_t soundRegister;
   uint8_t soundByte;
   unsigned long playTime;
};

struct SoundEntry {
   uint16_t soundIndex;
   uint8_t audioType;
   uint8_t overrideVolume;
   unsigned long playTime;
};

// These SoundEFfectEntry & Queue functions parcel out FX to the
// built-in sound card because it can only handle one sound
// at a time.
struct SoundEffectEntry {
   uint16_t soundEffectNum;
   unsigned long requestedPlayTime;
   unsigned long playUntil;
   uint8_t priority; // 0 is least important, 100 is most
   bool inUse;
};

#define INVALID_SOUND_INDEX 0xFFFF

class AudioHandler {
 public:
   AudioHandler();
   virtual ~AudioHandler();

   bool InitDevices(uint8_t audioType);

   void OutputTracksPlaying();

   void SetSoundFXVolume(uint8_t s_volume);
   void SetNotificationsVolume(uint8_t s_volume);
   void SetMusicVolume(uint8_t s_volume);
   void SetMusicDuckingGain(uint8_t s_ducking);
   void SetSoundFXDuckingGain(uint8_t s_ducking);

   bool PlayBackgroundSoundtrack(AudioSoundtrack* soundtrackArray, uint16_t numSoundtrackEntries, unsigned long currentTime,
                                 bool randomOrder = true);
   bool PlayBackgroundSong(uint16_t trackIndex, bool loopTrack = true);

   bool PlaySound(uint16_t soundIndex, uint8_t audioType, uint8_t overrideVolume = 0xFF);
   bool FadeSound(uint16_t soundIndex, int fadeGain, int numMilliseconds, bool stopTrack);

   bool QueueSound(uint16_t soundIndex, uint8_t audioType, unsigned long timeToPlay, uint8_t overrideVolume = 0xFF);
   inline bool QueueOriginalSound(uint16_t soundIndex, unsigned long timeToPlay, uint8_t overrideVolume = 0xFF) {
      return QueueSound(soundIndex, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, timeToPlay, overrideVolume);
   }
   inline bool QueueWavTriggerSound(uint16_t soundIndex, unsigned long timeToPlay, uint8_t overrideVolume = 0xFF) {
      return QueueSound(soundIndex, AUDIO_PLAY_TYPE_WAV_TRIGGER, timeToPlay, overrideVolume);
   }

   bool QueueSoundCardCommand(uint8_t scFunction, uint8_t scRegister, uint8_t scData, unsigned long startTime);

   bool PlaySoundCardWhenPossible(uint16_t soundEffectNum, unsigned long currentTime, unsigned long requestedPlayTime = 0,
                                  unsigned long playUntil = 50, uint8_t priority = 10);

   bool QueuePrioritizedNotification(uint16_t notificationIndex, uint16_t notificationLength, uint8_t priority, unsigned long currentTime);

   bool Update(unsigned long currentTime);

   bool StopSound(uint16_t soundIndex);
   bool StopCurrentNotification(uint8_t priority = 10);
   bool StopAllMusic();
   bool StopAllNotifications(uint8_t priority = 10);
   bool StopAllSoundFX();
   bool StopAllAudio();

 private:
   static constexpr int NUMBER_OF_SONGS_REMEMBERED = 10;
   static constexpr int VOICE_NOTIFICATION_STACK_SIZE = 5;
   static constexpr int SOUND_QUEUE_SIZE = 30;
   static constexpr int SOUND_CARD_QUEUE_SIZE = 100;
   static constexpr int SOUND_EFFECT_QUEUE_SIZE = 50;

   AudioSoundtrack* curSoundtrack;
   int soundFXGain;
   int notificationsGain;
   int musicGain;
   int musicDucking;
   int soundFXDucking;
   uint8_t voiceNotificationStackFirst;
   uint8_t voiceNotificationStackLast;
   uint8_t voiceNotificationPriorityStack[VOICE_NOTIFICATION_STACK_SIZE];
   uint8_t currentNotificationPriority;
   bool soundtrackRandomOrder;
   unsigned int currentNotificationPlaying;
   unsigned int voiceNotificationNumStack[VOICE_NOTIFICATION_STACK_SIZE];
   unsigned int voiceNotificationDuration[VOICE_NOTIFICATION_STACK_SIZE];
   unsigned int lastSongsPlayed[NUMBER_OF_SONGS_REMEMBERED];
   unsigned long currentNotificationStartTime;
   unsigned long nextSoundtrackPlayTime;
   uint16_t curSoundtrackEntries;
   uint16_t currentBackgroundTrack;

   SoundEntry soundQueue[SOUND_QUEUE_SIZE];

   unsigned long nextVoiceNotificationPlayTime;
   unsigned long backgroundSongEndTime;

#if defined(RPU_OS_USE_WTYPE_1_SOUND) || defined(RPU_OS_USE_WTYPE_2_SOUND)
   SoundEffectEntry CurrentSoundPlaying;
   SoundEffectEntry SoundEffectQueue[SOUND_EFFECT_QUEUE_SIZE];
#endif

#ifdef RPU_OS_USE_SB300
   SoundCardCommandEntry soundCardQueue[SOUND_CARD_QUEUE_SIZE];
#endif

#if defined(AUDIOHANDLER_USES_WAV_TRIGGER)
   WavTrigger wTrig; // Our WAV Trigger object
#endif

   int ConvertVolumeSettingToGain(uint8_t volumeSetting);

   void InitSB300Registers();
   void PlaySB300StartupBeep();
   void DuckCurrentSoundEffects();

   int SpaceLeftOnNotificationStack();
   void ClearSoundQueue();
   void ClearSoundCardQueue();
   void ClearNotificationStack(uint8_t priority = 10);
   void InitSoundEffectQueue();
   void StartNextSoundtrackSong(unsigned long currentTime);
   void ManageBackgroundSong(unsigned long currentTime);
   bool ServiceNotificationQueue(unsigned long currentTime);
   void PushToNotificationStack(unsigned int notification, unsigned int duration, uint8_t priority);
   uint8_t GetTopNotificationPriority();
   bool ServiceSoundCardQueue(unsigned long currentTime);
   bool ServiceSoundQueue(unsigned long currentTime);
};

#define AUDIO_HANDLER_H
#endif
