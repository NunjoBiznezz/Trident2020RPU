#include "WavTriggerHandler.h"

#if defined(RPU_OS_USE_WAV_TRIGGER)

#include "WavTrigger.h"
#include <Arduino.h>

#if (RPU_OS_HARDWARE_REV <= 3)
#define WTSerial Serial
#else
#define WTSerial Serial1
#endif

static const int VolumeToGainConversion[11] = {-70, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0};

WavTriggerHandler::WavTriggerHandler()
    : wTrig_(new WavTrigger(WTSerial)),
      soundFXGain_(0),
      notificationsGain_(0),
      musicGain_(0),
      musicDucking_(20),
      soundFXDucking_(20),
      currentBackgroundTrack_(BACKGROUND_TRACK_NONE),
      curSoundtrack_(nullptr),
      curSoundtrackEntries_(0),
      soundtrackRandomOrder_(true),
      backgroundSongEndTime_(0),
      currentNotificationPriority_(0),
      currentNotificationPlaying_(INVALID_SOUND_INDEX),
      currentNotificationStartTime_(0),
      nextVoiceNotificationPlayTime_(0) {
   for (int i = 0; i < NUMBER_OF_SONGS_REMEMBERED; i++) {
      lastSongsPlayed_[i] = BACKGROUND_TRACK_NONE;
   }
   clearSoundQueue();
}

WavTriggerHandler::~WavTriggerHandler() {
   delete wTrig_;
}

void WavTriggerHandler::init() {
   wTrig_->start();
   delay(10);
   wTrig_->stopAllTracks();
   wTrig_->samplerateOffset(0);
   wTrig_->setReporting(true);
}

int WavTriggerHandler::convertVolumeSettingToGain(uint8_t setting) {
   if (setting == 0) return -70;
   if (setting > 10) return 0;
   return VolumeToGainConversion[setting];
}

void WavTriggerHandler::setSoundFXVolume(uint8_t setting) {
   soundFXGain_ = convertVolumeSettingToGain(setting);
}

void WavTriggerHandler::setMusicVolume(uint8_t setting) {
   musicGain_ = convertVolumeSettingToGain(setting);
}

void WavTriggerHandler::setNotificationsVolume(uint8_t setting) {
   notificationsGain_ = convertVolumeSettingToGain(setting);
}

void WavTriggerHandler::setMusicDuckingGain(uint8_t ducking) {
   musicDucking_ = ducking;
}

void WavTriggerHandler::setSoundFXDuckingGain(uint8_t ducking) {
   soundFXDucking_ = ducking;
}

bool WavTriggerHandler::playSound(uint16_t trackIndex, uint8_t overrideVolume) {
   int gain = soundFXGain_;
   if (currentNotificationPlaying_ != INVALID_SOUND_INDEX) {
      gain -= soundFXDucking_;
   }
   if (overrideVolume != 0xFF) {
      gain = convertVolumeSettingToGain(overrideVolume);
   }
   wTrig_->trackStop(trackIndex);
   wTrig_->trackPlayPoly(trackIndex);
   wTrig_->trackGain(trackIndex, gain);
   return true;
}

bool WavTriggerHandler::stopSound(uint16_t trackIndex) {
   wTrig_->trackStop(trackIndex);
   return false;
}

bool WavTriggerHandler::fadeSound(uint16_t trackIndex, int fadeGain, int numMs, bool stopTrack) {
   wTrig_->trackFade(trackIndex, fadeGain, numMs, stopTrack);
   return true;
}

bool WavTriggerHandler::queueSound(uint16_t trackIndex, unsigned long playTime, uint8_t overrideVolume) {
   for (int i = 0; i < SOUND_QUEUE_SIZE; i++) {
      if (soundQueue_[i].playTime == 0) {
         soundQueue_[i].trackIndex     = trackIndex;
         soundQueue_[i].overrideVolume = overrideVolume;
         soundQueue_[i].playTime       = playTime;
         return true;
      }
   }
   return false;
}

void WavTriggerHandler::clearSoundQueue() {
   for (int i = 0; i < SOUND_QUEUE_SIZE; i++) {
      soundQueue_[i].playTime = 0;
   }
}

void WavTriggerHandler::serviceSoundQueue(unsigned long currentTime) {
   for (int i = 0; i < SOUND_QUEUE_SIZE; i++) {
      if (soundQueue_[i].playTime != 0 && soundQueue_[i].playTime < currentTime) {
         playSound(soundQueue_[i].trackIndex, soundQueue_[i].overrideVolume);
         soundQueue_[i].playTime = 0;
      }
   }
}

bool WavTriggerHandler::stopAllSoundFX() {
   wTrig_->stopAllTracks();
   return false;
}

void WavTriggerHandler::stopAllAudio() {
   wTrig_->stopAllTracks();
   notificationQueue_.clearAll();
   clearSoundQueue();
   currentBackgroundTrack_      = BACKGROUND_TRACK_NONE;
   curSoundtrack_               = nullptr;
   currentNotificationPlaying_  = INVALID_SOUND_INDEX;
   currentNotificationPriority_ = 0;
   nextVoiceNotificationPlayTime_ = 0;
   backgroundSongEndTime_       = 0;
}

bool WavTriggerHandler::stopAllMusic() {
   curSoundtrack_ = nullptr;
   if (currentBackgroundTrack_ != BACKGROUND_TRACK_NONE) {
      wTrig_->trackStop(currentBackgroundTrack_);
      currentBackgroundTrack_ = BACKGROUND_TRACK_NONE;
      return true;
   }
   currentBackgroundTrack_ = BACKGROUND_TRACK_NONE;
   return false;
}

bool WavTriggerHandler::playBackgroundSong(uint16_t trackIndex, bool loop) {
   stopAllMusic();
   if (trackIndex == BACKGROUND_TRACK_NONE) {
      return false;
   }
   currentBackgroundTrack_ = trackIndex;
   wTrig_->trackPlayPoly(trackIndex, true);
   if (loop) {
      wTrig_->trackLoop(trackIndex, true);
   }
   wTrig_->trackGain(trackIndex, musicGain_);
   return true;
}

bool WavTriggerHandler::playBackgroundSoundtrack(AudioSoundtrack* soundtrackArray,
                                                   uint16_t numEntries,
                                                   unsigned long currentTime,
                                                   bool randomOrder) {
   stopAllMusic();
   if (soundtrackArray == nullptr) {
      return false;
   }
   curSoundtrack_ = soundtrackArray;
   curSoundtrackEntries_ = numEntries;
   soundtrackRandomOrder_ = randomOrder;
   backgroundSongEndTime_ = (currentTime != 0) ? currentTime - 1 : 0;
   return true;
}

bool WavTriggerHandler::stopCurrentNotification(uint8_t priority) {
   nextVoiceNotificationPlayTime_ = 0;
   if (currentNotificationPlaying_ != INVALID_SOUND_INDEX && currentNotificationPriority_ <= priority) {
      wTrig_->trackStop(currentNotificationPlaying_);
      nextVoiceNotificationPlayTime_ = 1;
      currentNotificationPriority_ = 0;
      return true;
   }
   return false;
}

bool WavTriggerHandler::stopAllNotifications(uint8_t priority) {
   if (priority >= 10) {
      notificationQueue_.clearAll();
   } else {
      notificationQueue_.clearUpToPriority(priority);
   }
   return stopCurrentNotification(priority);
}

void WavTriggerHandler::duckCurrentSoundEffects() {
   if (!WavTrigger::hasSerialRx) {
      return;
   }
   for (int count = 0; count < WavTrigger::maxNumVoices(); count++) {
      int trackNum = wTrig_->getPlayingTrack(count);
      if (trackNum != ((int)0xFFFF) &&
          trackNum != ((int)currentBackgroundTrack_) &&
          trackNum != ((int)currentNotificationPlaying_)) {
         wTrig_->trackFade(trackNum, soundFXGain_ - soundFXDucking_, 500, 0);
      }
   }
}

bool WavTriggerHandler::queuePrioritizedNotification(uint16_t index, uint16_t length,
                                                      uint8_t priority,
                                                      unsigned long currentTime) {
   if (priority > notificationQueue_.topPriority()) {
      notificationQueue_.clearAll();
   }

   if (currentNotificationPlaying_ == INVALID_SOUND_INDEX) {
      if (currentBackgroundTrack_ != BACKGROUND_TRACK_NONE) {
         wTrig_->trackFade(currentBackgroundTrack_, musicGain_ - musicDucking_, 500, 0);
      }
      duckCurrentSoundEffects();
      nextVoiceNotificationPlayTime_ = (length != 0) ? currentTime + (unsigned long)length : 0;
      wTrig_->trackPlayPoly(index);
      wTrig_->trackGain(index, notificationsGain_);
      currentNotificationStartTime_ = currentTime;
      currentNotificationPlaying_ = index;
      currentNotificationPriority_ = priority;
   } else {
      notificationQueue_.push(index, length, priority);
   }
   return true;
}

void WavTriggerHandler::outputTracksPlaying() {
#if defined(DEBUG_PORT)
   char buf[256];
   DEBUG_PORT.write("Looking for playing tracks\n");
   wTrig_->getVersion(buf, 256);
   DEBUG_PORT.write("Version: ");
   DEBUG_PORT.write(buf);
   DEBUG_PORT.write("\n");
   for (int i = 0; i < 1000; i++) {
      if (wTrig_->isTrackPlaying(i)) {
         sprintf(buf, "Track %d playing\n", i);
         DEBUG_PORT.write(buf);
      }
   }
#endif
}

bool WavTriggerHandler::serviceNotificationQueue(unsigned long currentTime) {
   bool queueStillHasEntries = true;
   bool playNextNotification = false;

   if (nextVoiceNotificationPlayTime_ != 0) {
      if (currentTime > nextVoiceNotificationPlayTime_) {
         playNextNotification = true;
      }
   } else {
      if (currentNotificationPlaying_ != INVALID_SOUND_INDEX &&
          !wTrig_->isTrackPlaying(currentNotificationPlaying_)) {
         if (currentTime > (currentNotificationStartTime_ + 100)) {
            playNextNotification = true;
         }
      }
   }

   if (playNextNotification) {
      const NotificationEntry next = notificationQueue_.pull();

      if (next.notificationNum != INVALID_NOTIFICATION) {
         if (currentBackgroundTrack_ != BACKGROUND_TRACK_NONE) {
            wTrig_->trackFade(currentBackgroundTrack_, musicGain_ - musicDucking_, 500, 0);
         }
         duckCurrentSoundEffects();
         nextVoiceNotificationPlayTime_ = (next.duration != 0) ? currentTime + (unsigned long)next.duration : 0;
         wTrig_->trackPlayPoly(next.notificationNum);
         wTrig_->trackGain(next.notificationNum, notificationsGain_);
         currentNotificationStartTime_ = currentTime;
         currentNotificationPlaying_ = next.notificationNum;
         currentNotificationPriority_ = next.priority;
      } else {
         if (currentBackgroundTrack_ != BACKGROUND_TRACK_NONE) {
            wTrig_->trackFade(currentBackgroundTrack_, musicGain_, 1500, 0);
         }
         nextVoiceNotificationPlayTime_ = 0;
         currentNotificationPlaying_ = INVALID_SOUND_INDEX;
         currentNotificationPriority_ = 0;
         queueStillHasEntries = false;
      }
   }

   return queueStillHasEntries;
}

void WavTriggerHandler::startNextSoundtrackSong(unsigned long currentTime) {
   unsigned int retSong = currentTime % curSoundtrackEntries_;
   bool songRecentlyPlayed = false;

   for (unsigned int songCount = 0; songCount < curSoundtrackEntries_; songCount++) {
      for (uint8_t count = 0; count < NUMBER_OF_SONGS_REMEMBERED; count++) {
         if (lastSongsPlayed_[count] == curSoundtrack_[retSong].TrackIndex) {
            songRecentlyPlayed = true;
            break;
         }
      }
      if (!songRecentlyPlayed) break;
      songRecentlyPlayed = false;
      if (++retSong >= curSoundtrackEntries_) retSong = 0;
   }

   for (uint8_t count = NUMBER_OF_SONGS_REMEMBERED - 1; count > 0; count--) {
      lastSongsPlayed_[count] = lastSongsPlayed_[count - 1];
   }
   lastSongsPlayed_[0] = curSoundtrack_[retSong].TrackIndex;

   backgroundSongEndTime_ = ((unsigned long)curSoundtrack_[retSong].TrackLength * 1000) + currentTime;

   if (currentBackgroundTrack_ != BACKGROUND_TRACK_NONE) {
      wTrig_->trackFade(currentBackgroundTrack_, -80, 2000, 1);
   }
   currentBackgroundTrack_ = curSoundtrack_[retSong].TrackIndex;
   wTrig_->trackPlayPoly(currentBackgroundTrack_, true);
   wTrig_->trackGain(currentBackgroundTrack_, musicGain_);
}

void WavTriggerHandler::manageBackgroundSong(unsigned long currentTime) {
   if (curSoundtrack_ == nullptr) return;

   if (backgroundSongEndTime_ != 0) {
      if (currentTime >= backgroundSongEndTime_) {
         startNextSoundtrackSong(currentTime);
      }
   } else {
      if (!wTrig_->isTrackPlaying(currentBackgroundTrack_)) {
         startNextSoundtrackSong(currentTime);
      }
   }
}

bool WavTriggerHandler::update(unsigned long currentTime) {
   wTrig_->update();
   serviceSoundQueue(currentTime);
   manageBackgroundSong(currentTime);
   return serviceNotificationQueue(currentTime);
}

#endif // RPU_OS_USE_WAV_TRIGGER
