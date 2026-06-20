/****************************************************************
 * Class - AudioHandler
 *
 * This class wraps the different audio output options for
 * pinball machines (WAV Trigger, SB-100, SB-300, S&T
 * -51, etc.) to provide different output options. Additionally,
 * it adds a bunch of audio management features:
 *
 *   1) Different volume controls for FX, callouts, and music
 *   2) Automatically ducks music behind callouts
 *   3) Supports background soundtracks or looping songs
 *   4) A sound can be queued to play at a future time
 *   5) Callouts can be given different priorities
 *   6) Queued callouts at the same priority will be stacked
 *
 *
 * Typical usage:
 *   A global variable of class "AudioHandler" is declared
 *     (ex: "AudioHandler Audio;")
 *
 *   During the setup() function:
 *    Audio.InitDevices(AUDIO_PLAY_TYPE_WAV_TRIGGER | AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS); // declare what type of audio should be supported
 *    Audio.StopAllAudio(); // Stop any currently playing sounds
 *    Audio.SetMusicDuckingGain(12); // negative gain applied to music when callouts are being played
 *    Audio.QueueSound(SOUND_EFFECT_MACHINE_INTRO, AUDIO_PLAY_TYPE_WAV_TRIGGER, CurrentTime+1200); // Play machine startup sound in 1.2 s
 *
 *   Volumes set (pulled from EEPROM or set in setup routine):
 *    Audio.SetMusicVolume(MusicVolume); // value from 0-10 (inclusive)
 *    Audio.SetSoundFXVolume(SoundEffectsVolume); // value from 0-10 (inclusive)
 *    Audio.SetNotificationsVolume(CalloutsVolume); // value from 0-10 (inclusive)
 *
 *   During the loop():
 *    Audio.Update(CurrentTime);
 *
 *   During game play:
 *    Audio.PlayBackgroundSong(songNum, true); // loop a background song
 *    Audio.PlaySound(soundEffectNum, AUDIO_PLAY_TYPE_WAV_TRIGGER); // play sound effect through wav trigger
 *    Audio.QueueSound(0x02, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime); // Queue sound card command for now
 *    Audio.QueueNotification(soundEffectNum, VoiceNotificationDurations[soundEffectNum-SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START],
 * priority, CurrentTime); // Queue notification
 *
 *   End of ball:
 *    Audio.StopAllAudio(); // Stop audio
 */

#include "AudioHandler.h"
#include "../include/RPU.h"
#include "RPU_config.h"
#include <Arduino.h>

constexpr uint16_t VOICE_NOTIFICATION_STACK_EMPTY = 0xFFFF;
constexpr uint16_t BACKGROUND_TRACK_NONE = 0xFFFF;
constexpr uint16_t INVALID_SOUND_INDEX = 0xFFFF;

#define SB300_SOUND_FUNCTION_SQUARE_WAVE 0
#define SB300_SOUND_FUNCTION_ANALOG 1

const int volumeToGainConversion[11] = {-70, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0};

AudioHandler::AudioHandler() {
   curSoundtrack = NULL;
   curSoundtrackEntries = 0;
   soundFXGain = 0;
   notificationsGain = 0;
   musicGain = 0;
   clearSoundQueue();
   clearSoundCardQueue();
   clearNotificationStack();
   currentBackgroundTrack = BACKGROUND_TRACK_NONE;
   soundtrackRandomOrder = true;
   nextSoundtrackPlayTime = 0;
   backgroundSongEndTime = 0;
   nextVoiceNotificationPlayTime = 0;

   voiceNotificationStackFirst = 0;
   voiceNotificationStackLast = 0;
   currentNotificationPriority = 0;
   currentNotificationPlaying = INVALID_SOUND_INDEX;
   musicDucking = 20;
   soundFXDucking = 20;

   for (int count = 0; count < NUMBER_OF_SONGS_REMEMBERED; count++) {
      lastSongsPlayed[count] = BACKGROUND_TRACK_NONE;
   }

}

AudioHandler::~AudioHandler() {}

bool AudioHandler::InitDevices(uint8_t audioType) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   if (audioType & AUDIO_PLAY_TYPE_WAV_TRIGGER) {
      // WAV Trigger startup at 57600
      wTrig.start();
      delay(10);
      wTrig.stopAllTracks();
      wTrig.samplerateOffset(0);
      wTrig.setReporting(true);
   }
#endif

   if (audioType & AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS) {
#ifdef RPU_OS_USE_SB300
      initSB300Registers();
      playSB300StartupBeep();
#endif
   }

   return true;
}

int AudioHandler::convertVolumeSettingToGain(uint8_t volumeSetting) {
   if (volumeSetting == 0) {
      return -70;
   }
   if (volumeSetting > 10) {
      return 0;
   }
   return volumeToGainConversion[volumeSetting];
}

void AudioHandler::SetSoundFXVolume(uint8_t s_volume) {
   soundFXGain = convertVolumeSettingToGain(s_volume);
}

void AudioHandler::SetNotificationsVolume(uint8_t s_volume) {
   notificationsGain = convertVolumeSettingToGain(s_volume);
}

void AudioHandler::SetMusicVolume(uint8_t s_volume) {
   musicGain = convertVolumeSettingToGain(s_volume);
}

void AudioHandler::SetMusicDuckingGain(uint8_t s_ducking) {
   musicDucking = s_ducking;
}

void AudioHandler::SetSoundFXDuckingGain(uint8_t s_ducking) {
   soundFXDucking = s_ducking;
}

bool AudioHandler::StopSound(uint16_t soundIndex) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wTrig.trackStop(soundIndex);
#else
   (void)soundIndex;
#endif
   return false;
}

bool AudioHandler::StopAllMusic() {
   curSoundtrack = NULL;
#if defined(RPU_OS_USE_WAV_TRIGGER)
   if (currentBackgroundTrack != BACKGROUND_TRACK_NONE) {
      wTrig.trackStop(currentBackgroundTrack);
      currentBackgroundTrack = BACKGROUND_TRACK_NONE;
      return true;
   }
#endif
   currentBackgroundTrack = BACKGROUND_TRACK_NONE;
   return false;
}

void AudioHandler::clearNotificationStack(uint8_t priority) {
   if (priority == 10) {
      voiceNotificationStackFirst = 0;
      voiceNotificationStackLast = 0;
      for (uint8_t count = 0; count < VOICE_NOTIFICATION_STACK_SIZE; count++) {
         voiceNotificationNumStack[count] = VOICE_NOTIFICATION_STACK_EMPTY;
         voiceNotificationDuration[count] = 0;
         voiceNotificationPriorityStack[count] = 0;
      }
   } else {
      uint8_t tempFirst = voiceNotificationStackFirst;
      uint8_t tempLast = voiceNotificationStackLast;
      while (tempFirst != tempLast) {
         if (voiceNotificationPriorityStack[tempFirst] <= priority) {
            voiceNotificationNumStack[tempFirst] = INVALID_SOUND_INDEX;
         }
         tempFirst += 1;
         if (tempFirst >= VOICE_NOTIFICATION_STACK_SIZE) {
            tempFirst = 0;
         }
      }
   }
}

bool AudioHandler::StopCurrentNotification(uint8_t priority) {
   nextVoiceNotificationPlayTime = 0;

   if (currentNotificationPlaying != INVALID_SOUND_INDEX && currentNotificationPriority <= priority) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wTrig.trackStop(currentNotificationPlaying);
#endif
      //    currentNotificationPlaying = INVALID_SOUND_INDEX;
      nextVoiceNotificationPlayTime = 1;
      currentNotificationPriority = 0;
      return true;
   }
   return false;
}

int AudioHandler::spaceLeftOnNotificationStack() {
   if (voiceNotificationStackFirst >= VOICE_NOTIFICATION_STACK_SIZE || voiceNotificationStackLast >= VOICE_NOTIFICATION_STACK_SIZE) {
      return 0;
   }
   if (voiceNotificationStackLast >= voiceNotificationStackFirst) {
      return ((VOICE_NOTIFICATION_STACK_SIZE - 1) - (voiceNotificationStackLast - voiceNotificationStackFirst));
   }
   return (voiceNotificationStackFirst - voiceNotificationStackLast) - 1;
}

void AudioHandler::pushToNotificationStack(unsigned int notification, unsigned int duration, uint8_t priority) {
   // If the switch stack last index is out of range, then it's an error - return
   if (spaceLeftOnNotificationStack() == 0) {
      return;
   }

   voiceNotificationNumStack[voiceNotificationStackLast] = notification;
   voiceNotificationDuration[voiceNotificationStackLast] = duration;
   voiceNotificationPriorityStack[voiceNotificationStackLast] = priority;

   voiceNotificationStackLast += 1;
   if (voiceNotificationStackLast >= VOICE_NOTIFICATION_STACK_SIZE) {
      // If the end index is off the end, then wrap
      voiceNotificationStackLast = 0;
   }
}

uint8_t AudioHandler::getTopNotificationPriority() {
   uint8_t startStack = voiceNotificationStackFirst;
   uint8_t endStack = voiceNotificationStackLast;
   if (startStack == endStack) {
      return 0;
   }

   uint8_t topPriorityFound = 0;

   while (startStack != endStack) {
      if (voiceNotificationPriorityStack[startStack] > topPriorityFound) {
         topPriorityFound = voiceNotificationPriorityStack[startStack];
      }
      startStack += 1;
      if (startStack >= VOICE_NOTIFICATION_STACK_SIZE) {
         startStack = 0;
      }
   }

   return topPriorityFound;
}

void AudioHandler::duckCurrentSoundEffects() {
   // We can't duck if we don't have bi-directional communication
   // So <=3 revs have to return
   if (RPU_OS_HARDWARE_REV <= 3) {
      return;
   }

#if defined(RPU_OS_USE_WAV_TRIGGER)
   for (int count = 0; count < WavTrigger::maxNumVoices(); count++) {
      int trackNum = wTrig.getPlayingTrack(count);
      if (trackNum != ((int)0xFFFF) && trackNum != ((int)currentBackgroundTrack) && trackNum != ((int)currentNotificationPlaying)) {
         // This is a sound effect that needs to be ducked
         wTrig.trackFade(trackNum, soundFXGain - soundFXDucking, 500, 0);
      }
   }
#endif
}

bool AudioHandler::QueuePrioritizedNotification(uint16_t notificationIndex, uint16_t notificationLength, uint8_t priority,
                                                unsigned long currentTime) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   // if everything on the queue has a lower priority, kill all those
   uint8_t topQueuePriority = getTopNotificationPriority();
   if (priority > topQueuePriority) {
      clearNotificationStack();
   }

   // If there's nothing playing, we can play it now
   if (currentNotificationPlaying == INVALID_SOUND_INDEX) {
      if (currentBackgroundTrack != BACKGROUND_TRACK_NONE) {
         wTrig.trackFade(currentBackgroundTrack, musicGain - musicDucking, 500, 0);
      }
      duckCurrentSoundEffects();
      if (notificationLength) {
         nextVoiceNotificationPlayTime = currentTime + (unsigned long)(notificationLength);
      } else {
         nextVoiceNotificationPlayTime = 0;
      }

      wTrig.trackPlayPoly(notificationIndex);
      wTrig.trackGain(notificationIndex, notificationsGain);
      currentNotificationStartTime = currentTime;

      currentNotificationPlaying = notificationIndex;
      currentNotificationPriority = priority;
   } else {
      pushToNotificationStack(notificationIndex, notificationLength, priority);
   }
#else
   // Phony stuff to get rid of warnings
   (void)notificationIndex;
   (void)notificationLength;
   (void)priority;
   (void)currentTime;
#endif

   return true;
}

void AudioHandler::OutputTracksPlaying() {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   int i;
   char buf[256];
   sprintf(buf, "nothing");
   Serial.write("Looking for playing tracks\n");
   wTrig.getVersion(buf, 256);
   Serial.write("Version: ");
   Serial.write(buf);
   Serial.write("\n");
   for (i = 0; i < 1000; i++) {
      if (wTrig.isTrackPlaying(i)) {
         sprintf(buf, "Track %d playing\n", i);
         Serial.write(buf);
      }
   }
#endif
}

bool AudioHandler::serviceNotificationQueue(unsigned long currentTime) {
   bool queueStillHasEntries = true;
#if defined(RPU_OS_USE_WAV_TRIGGER)
   bool playNextNotification = false;

   if (nextVoiceNotificationPlayTime != 0) {
      if (currentTime > nextVoiceNotificationPlayTime) {
         playNextNotification = true;
      }
   } else {
      if (currentNotificationPlaying != INVALID_SOUND_INDEX && !wTrig.isTrackPlaying(currentNotificationPlaying)) {
         if (currentTime > (currentNotificationStartTime + 100)) {
            playNextNotification = true;
         }
      }
   }

   if (playNextNotification) {
      uint8_t nextPriority = 0;
      unsigned int nextNotification = VOICE_NOTIFICATION_STACK_EMPTY;
      unsigned int nextDuration = 0;

      // Current notification done, see if there's another
      if (voiceNotificationStackLast >= VOICE_NOTIFICATION_STACK_SIZE) {
         voiceNotificationStackLast = (VOICE_NOTIFICATION_STACK_SIZE - 1);
      }
      while (voiceNotificationStackFirst != voiceNotificationStackLast) {
         nextPriority = voiceNotificationPriorityStack[voiceNotificationStackFirst];
         nextNotification = voiceNotificationNumStack[voiceNotificationStackFirst];
         nextDuration = voiceNotificationDuration[voiceNotificationStackFirst];

         voiceNotificationStackFirst += 1;
         if (voiceNotificationStackFirst >= VOICE_NOTIFICATION_STACK_SIZE) {
            voiceNotificationStackFirst = 0;
         }
         if (nextNotification != INVALID_SOUND_INDEX) {
            break;
         }
      }

      if (nextNotification != VOICE_NOTIFICATION_STACK_EMPTY) {
         if (currentBackgroundTrack != BACKGROUND_TRACK_NONE) {
            wTrig.trackFade(currentBackgroundTrack, musicGain - musicDucking, 500, 0);
         }
         duckCurrentSoundEffects();
         if (nextDuration != 0) {
            nextVoiceNotificationPlayTime = currentTime + (unsigned long)(nextDuration);
         } else {
            nextVoiceNotificationPlayTime = 0;
         }
         wTrig.trackPlayPoly(nextNotification);
         wTrig.trackGain(nextNotification, notificationsGain);
         currentNotificationStartTime = currentTime;
         currentNotificationPlaying = nextNotification;
         currentNotificationPriority = nextPriority;
      } else {
         // No more notifications -- set the volume back up and clear the variable
         if (currentBackgroundTrack != BACKGROUND_TRACK_NONE) {
            wTrig.trackFade(currentBackgroundTrack, musicGain, 1500, 0);
         }
         nextVoiceNotificationPlayTime = 0;
         currentNotificationPlaying = INVALID_SOUND_INDEX;
         currentNotificationPriority = 0;
         queueStillHasEntries = false;
      }
   }
#else
   (void)currentTime;
#endif

   return queueStillHasEntries;
}

bool AudioHandler::StopAllNotifications(uint8_t priority) {
   clearNotificationStack(priority);
   return StopCurrentNotification(priority);
}

bool AudioHandler::StopAllSoundFX() {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wTrig.stopAllTracks();
#endif
   clearSoundCardQueue();
   clearSoundQueue();
   return false;
}

bool AudioHandler::StopAllAudio() {
   bool anythingPlaying = false;
   if (StopAllMusic()) {
      anythingPlaying = true;
   }
   if (StopAllNotifications()) {
      anythingPlaying = true;
   }
   if (StopAllSoundFX()) {
      anythingPlaying = true;
   }
   return anythingPlaying;
}

#ifdef RPU_OS_USE_SB300
void AudioHandler::initSB300Registers() {
#if (RPU_OS_HARDWARE_REV >= 2)
   RPU_PlaySB300SquareWave(1, 0x00); // Write 0x00 to CR2 (Timer 2 off, continuous mode, 16-bit, C2 clock, CR3 set)
   RPU_PlaySB300SquareWave(0, 0x00); // Write 0x00 to CR3 (Timer 3 off, continuous mode, 16-bit, C3 clock, not prescaled)
   RPU_PlaySB300SquareWave(1, 0x01); // Write 0x00 to CR2 (Timer 2 off, continuous mode, 16-bit, C2 clock, CR1 set)
   RPU_PlaySB300SquareWave(0, 0x00); // Write 0x00 to CR1 (Timer 1 off, continuous mode, 16-bit, C1 clock, timers allowed)
#endif
}

void AudioHandler::playSB300StartupBeep() {
#if (RPU_OS_HARDWARE_REV >= 2)
   RPU_PlaySB300SquareWave(1, 0x92); // Write 0x92 to CR2 (Timer 2 on, continuous mode, 16-bit, E clock, CR3 set)
   RPU_PlaySB300SquareWave(0, 0x92); // Write 0x92 to CR3 (Timer 3 on, continuous mode, 16-bit, E clock, not prescaled)
   RPU_PlaySB300SquareWave(4, 0x02); // Set Timer 2 to 0x0200
   RPU_PlaySB300SquareWave(5, 0x00);
   RPU_PlaySB300SquareWave(6, 0x80); // Set Timer 3 to 0x8000
   RPU_PlaySB300SquareWave(7, 0x00);
   RPU_PlaySB300Analog(0, 0x02);
#endif
}
#endif

void AudioHandler::clearSoundCardQueue() {
#ifdef RPU_OS_USE_SB300
   for (int count = 0; count < SOUND_CARD_QUEUE_SIZE; count++) {
      soundCardQueue[count].playTime = 0;
   }
#endif
}

void AudioHandler::clearSoundQueue() {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      soundQueue[count].playTime = 0;
   }
}

bool AudioHandler::PlaySound(uint16_t soundIndex, uint8_t audioType, uint8_t overrideVolume) {
   bool soundPlayed = false;
   int gain = soundFXGain;

   if (currentNotificationPlaying != INVALID_SOUND_INDEX) {
      // reduce gain (by ducking amount) if there's a notification playing
      gain -= soundFXDucking;
   }
   if (overrideVolume != 0xFF) {
      gain = convertVolumeSettingToGain(overrideVolume);
   }

   switch (audioType) {
   case AUDIO_PLAY_TYPE_CHIMES:
#if defined(RPU_OS_USE_SB100)
      //    RPU_PlaySB100Chime((uint8_t)soundIndex);
      soundPlayed = true;
#endif
      break;

   case AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS:
#ifdef RPU_OS_USE_DASH51
      RPU_PlaySoundDash51((uint8_t)soundIndex);
      soundPlayed = true;
#endif
#ifdef RPU_OS_USE_S_AND_T
      RPU_PlaySoundSAndT((uint8_t)soundIndex);
      soundPlayed = true;
#endif
#ifdef RPU_OS_USE_SB100
      RPU_PlaySB100((uint8_t)soundIndex);
      soundPlayed = true;
#endif
      break;

   case AUDIO_PLAY_TYPE_WAV_TRIGGER:
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wTrig.trackStop(soundIndex);  // 1.3

      wTrig.trackPlayPoly(soundIndex);
      wTrig.trackGain(soundIndex, gain);
      soundPlayed = true;
#endif
      break;
   }

   (void)gain;
   (void)soundIndex;

   return soundPlayed;
}

bool AudioHandler::FadeSound(uint16_t soundIndex, int fadeGain, int numMilliseconds, bool stopTrack) {
   bool soundFaded = false;
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wTrig.trackFade(soundIndex, fadeGain, numMilliseconds, stopTrack);
   soundFaded = true;
#endif
   (void)soundIndex;
   (void)fadeGain;
   (void)numMilliseconds;
   (void)stopTrack;
   return soundFaded;
}

bool AudioHandler::QueueSound(uint16_t soundIndex, uint8_t audioType, unsigned long timeToPlay, uint8_t overrideVolume) {
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundQueue[count].playTime == 0) {
         soundQueue[count].soundIndex = soundIndex;
         soundQueue[count].audioType = audioType;
         soundQueue[count].playTime = timeToPlay;
         soundQueue[count].overrideVolume = overrideVolume;
         return true;
      }
   }

   return false;
}

bool AudioHandler::QueueSoundCardCommand(uint8_t scFunction, uint8_t scRegister, uint8_t scData, unsigned long startTime) {
#ifdef RPU_OS_USE_SB300
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundCardQueue[count].playTime == 0) {
         soundCardQueue[count].soundFunction = scFunction;
         soundCardQueue[count].soundRegister = scRegister;
         soundCardQueue[count].soundByte = scData;
         soundCardQueue[count].playTime = startTime;
         return true;
      }
   }
#else
   // Phony stuff to get rid of warnings
   unsigned long totalval = scFunction + scRegister + scData + startTime;
   totalval += 1;
#endif
   return false;
}

bool AudioHandler::serviceSoundQueue(unsigned long currentTime) {
   bool soundCommandSent = false;
   for (int count = 0; count < SOUND_QUEUE_SIZE; count++) {
      if (soundQueue[count].playTime != 0 && soundQueue[count].playTime < currentTime) {
         PlaySound(soundQueue[count].soundIndex, soundQueue[count].audioType, soundQueue[count].overrideVolume);
         soundQueue[count].playTime = 0;
         soundCommandSent = true;
      }
   }

   return soundCommandSent;
}

bool AudioHandler::serviceSoundCardQueue(unsigned long currentTime) {
#if (RPU_OS_HARDWARE_REV >= 2 && defined(RPU_OS_USE_SB300))
   bool soundCommandSent = false;
   for (int count = 0; count < SOUND_CARD_QUEUE_SIZE; count++) {
      if (soundCardQueue[count].playTime != 0 && soundCardQueue[count].playTime < currentTime) {
         if (soundCardQueue[count].soundFunction == SB300_SOUND_FUNCTION_SQUARE_WAVE) {
            RPU_PlaySB300SquareWave(soundCardQueue[count].soundRegister, soundCardQueue[count].soundByte);
         } else if (soundCardQueue[count].soundFunction == SB300_SOUND_FUNCTION_ANALOG) {
            RPU_PlaySB300Analog(soundCardQueue[count].soundRegister, soundCardQueue[count].soundByte);
         }
         soundCardQueue[count].playTime = 0;
         soundCommandSent = true;
      }
   }

   return soundCommandSent;
#else
   // Phony stuff to get rid of warnings
   (void)currentTime;
   return false;
#endif
}

bool AudioHandler::PlayBackgroundSoundtrack(AudioSoundtrack* soundtrackArray, uint16_t numSoundtrackEntries, unsigned long currentTime,
                                            bool randomOrder) {
   StopAllMusic();
   if (soundtrackArray == NULL) {
      return false;
   }

   curSoundtrack = soundtrackArray;
   curSoundtrackEntries = numSoundtrackEntries;
   soundtrackRandomOrder = randomOrder;
   if (currentTime != 0) {
      backgroundSongEndTime = currentTime - 1;
   } else {
      backgroundSongEndTime = 0;
   }

   return true;
}

bool AudioHandler::PlayBackgroundSong(uint16_t trackIndex, bool loopTrack) {
   StopAllMusic();
   bool trackPlayed = false;

   if (trackIndex != BACKGROUND_TRACK_NONE) {
      currentBackgroundTrack = trackIndex;
#if defined(RPU_OS_USE_WAV_TRIGGER)
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wTrig.trackPlayPoly(trackIndex, true);
      trackPlayed = true;
#else
      wTrig.trackPlayPoly(trackIndex);
      trackPlayed = true;
#endif
      if (loopTrack) {
         wTrig.trackLoop(trackIndex, true);
      }
      wTrig.trackGain(trackIndex, musicGain);
#endif
   }
   (void)loopTrack;

   return trackPlayed;
}

void AudioHandler::startNextSoundtrackSong(unsigned long currentTime) {
   unsigned int retSong = (currentTime % curSoundtrackEntries);
   bool songRecentlyPlayed = false;

   unsigned int songCount = 0;
   for (songCount = 0; songCount < curSoundtrackEntries; songCount++) {
      for (uint8_t count = 0; count < NUMBER_OF_SONGS_REMEMBERED; count++) {
         if (lastSongsPlayed[count] == curSoundtrack[retSong].TrackIndex) {
            songRecentlyPlayed = true;
            break;
         }
      }
      if (!songRecentlyPlayed) {
         break;
      }
      retSong = (retSong + 1);
      songRecentlyPlayed = false;
      if (retSong >= curSoundtrackEntries) {
         retSong = 0;
      }
   }

   // Record this song in the array
   for (uint8_t count = (NUMBER_OF_SONGS_REMEMBERED - 1); count > 0; count--) {
      lastSongsPlayed[count] = lastSongsPlayed[count - 1];
   }
   lastSongsPlayed[0] = curSoundtrack[retSong].TrackIndex;

   backgroundSongEndTime = (((unsigned long)curSoundtrack[retSong].TrackLength) * 1000) + currentTime;

   if (currentBackgroundTrack != BACKGROUND_TRACK_NONE) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wTrig.trackFade(currentBackgroundTrack, -80, 2000, 1);
#endif
   }
   currentBackgroundTrack = curSoundtrack[retSong].TrackIndex;

#if defined(RPU_OS_USE_WAV_TRIGGER)
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wTrig.trackPlayPoly(currentBackgroundTrack, true);
#else
   wTrig.trackPlayPoly(currentBackgroundTrack);
#endif
   wTrig.trackGain(currentBackgroundTrack, musicGain);
#endif
}

void AudioHandler::manageBackgroundSong(unsigned long currentTime) {
   if (curSoundtrack == NULL) {
      return;
   }

   if (backgroundSongEndTime != 0) {
      if (currentTime >= backgroundSongEndTime) {
         startNextSoundtrackSong(currentTime);
      }
   } else {
#if defined(RPU_OS_USE_WAV_TRIGGER)
      if (!wTrig.isTrackPlaying(currentBackgroundTrack)) {
         startNextSoundtrackSong(currentTime);
      }
#endif
   }
}

bool AudioHandler::Update(unsigned long currentTime) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wTrig.update();
#endif
   bool queueHasEntries = false;
   manageBackgroundSong(currentTime);
   serviceSoundQueue(currentTime);
   serviceSoundCardQueue(currentTime);
   if (serviceNotificationQueue(currentTime)) {
      queueHasEntries = true;
   }
   return queueHasEntries;
}
