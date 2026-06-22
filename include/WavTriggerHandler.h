#pragma once

#if defined(RPU_OS_USE_WAV_TRIGGER)

#include "NotificationQueue.h"
#include <stdint.h>

class WavTrigger;

struct AudioSoundtrack {
   uint16_t TrackIndex;
   uint16_t TrackLength;
};

// Owns the WavTrigger driver and all WAV-specific audio management:
// background music, voice notification queue, ducking, and gain control.
class WavTriggerHandler {
public:
   WavTriggerHandler();
   ~WavTriggerHandler();

   void init();
   // Returns true while the notification queue has pending entries.
   bool update(unsigned long currentTime);

   void setSoundFXVolume(uint8_t setting);
   void setMusicVolume(uint8_t setting);
   void setNotificationsVolume(uint8_t setting);
   void setMusicDuckingGain(uint8_t ducking);
   void setSoundFXDuckingGain(uint8_t ducking);

   bool playSound(uint16_t trackIndex, uint8_t overrideVolume = 0xFF);
   bool queueSound(uint16_t trackIndex, unsigned long playTime, uint8_t overrideVolume = 0xFF);
   bool stopSound(uint16_t trackIndex);
   bool fadeSound(uint16_t trackIndex, int fadeGain, int numMs, bool stopTrack);
   bool stopAllSoundFX();

   bool playBackgroundSong(uint16_t trackIndex, bool loop = true);
   bool playBackgroundSoundtrack(AudioSoundtrack* soundtrackArray, uint16_t numEntries,
                                  unsigned long currentTime, bool randomOrder = true);
   bool stopAllMusic();

   bool queuePrioritizedNotification(uint16_t index, uint16_t length, uint8_t priority,
                                      unsigned long currentTime);
   bool stopCurrentNotification(uint8_t priority = 10);
   bool stopAllNotifications(uint8_t priority = 10);
   void stopAllAudio();

   void outputTracksPlaying();

private:
   static constexpr int  NUMBER_OF_SONGS_REMEMBERED  = 10;
   static constexpr int  VOICE_NOTIFICATION_STACK_SIZE = 5;
   static constexpr int  SOUND_QUEUE_SIZE             = 8;
   static constexpr uint16_t BACKGROUND_TRACK_NONE   = 0xFFFF;
   static constexpr uint16_t INVALID_SOUND_INDEX      = 0xFFFF;
   static constexpr uint16_t INVALID_NOTIFICATION     = 0xFFFF;

   struct WavSoundEntry {
      uint16_t      trackIndex;
      uint8_t       overrideVolume;
      unsigned long playTime;
   };

   WavTrigger* wTrig_;

   int soundFXGain_;
   int notificationsGain_;
   int musicGain_;
   int musicDucking_;
   int soundFXDucking_;

   uint16_t currentBackgroundTrack_;
   AudioSoundtrack* curSoundtrack_;
   uint16_t curSoundtrackEntries_;
   bool soundtrackRandomOrder_;
   unsigned int lastSongsPlayed_[NUMBER_OF_SONGS_REMEMBERED];
   unsigned long backgroundSongEndTime_;

   WavSoundEntry soundQueue_[SOUND_QUEUE_SIZE];

   NotificationQueue<VOICE_NOTIFICATION_STACK_SIZE> notificationQueue_;
   uint8_t currentNotificationPriority_;
   unsigned int currentNotificationPlaying_;
   unsigned long currentNotificationStartTime_;
   unsigned long nextVoiceNotificationPlayTime_;

   int convertVolumeSettingToGain(uint8_t setting);
   void duckCurrentSoundEffects();
   void startNextSoundtrackSong(unsigned long currentTime);
   void manageBackgroundSong(unsigned long currentTime);
   bool serviceNotificationQueue(unsigned long currentTime);
   void clearSoundQueue();
   void serviceSoundQueue(unsigned long currentTime);
};

#endif // RPU_OS_USE_WAV_TRIGGER
