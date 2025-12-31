#ifndef AUDIO_HANDLER_H

#include "RPU.h"
#include "RPU_config.h"
#include <HardwareSerial.h>
#include <stdint.h>

// These are also used as masks in the InitDevices code, so leave them as powers of 2
constexpr uint8_t AUDIO_PLAY_TYPE_CHIMES = 1;
constexpr uint8_t AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS = 2;
constexpr uint8_t AUDIO_PLAY_TYPE_WAV_TRIGGER = 4;

constexpr int NUMBER_OF_SONGS_REMEMBERED = 10;

constexpr int VOICE_NOTIFICATION_STACK_SIZE = 5;
constexpr uint16_t VOICE_NOTIFICATION_STACK_EMPTY = 0xFFFF;

constexpr uint16_t BACKGROUND_TRACK_NONE = 0xFFFF;

struct AudioSoundtrack {
   unsigned short TrackIndex;
   unsigned short TrackLength;
};

#define SOUND_CARD_QUEUE_SIZE 100
#define SB300_SOUND_FUNCTION_SQUARE_WAVE 0
#define SB300_SOUND_FUNCTION_ANALOG 1
#define SOUND_EFFECT_QUEUE_SIZE 50

struct SoundCardCommandEntry {
   uint8_t soundFunction;
   uint8_t soundRegister;
   uint8_t soundByte;
   unsigned long playTime;
};

#define SOUND_QUEUE_SIZE 30

struct SoundEntry {
   unsigned short soundIndex;
   uint8_t audioType;
   uint8_t overrideVolume;
   unsigned long playTime;
};

// These SoundEFfectEntry & Queue functions parcel out FX to the
// built-in sound card because it can only handle one sound
// at a time.
struct SoundEffectEntry {
   unsigned short soundEffectNum;
   unsigned long requestedPlayTime;
   unsigned long playUntil;
   uint8_t priority; // 0 is least important, 100 is most
   bool inUse;
};

#define CMD_GET_VERSION 1
#define CMD_GET_SYS_INFO 2
#define CMD_TRACK_CONTROL 3
#define CMD_STOP_ALL 4
#define CMD_MASTER_VOLUME 5
#define CMD_TRACK_VOLUME 8
#define CMD_AMP_POWER 9
#define CMD_TRACK_FADE 10
#define CMD_RESUME_ALL_SYNC 11
#define CMD_SAMPLERATE_OFFSET 12
#define CMD_TRACK_CONTROL_EX 13
#define CMD_SET_REPORTING 14
#define CMD_SET_TRIGGER_BANK 15

#define TRK_PLAY_SOLO 0
#define TRK_PLAY_POLY 1
#define TRK_PAUSE 2
#define TRK_RESUME 3
#define TRK_STOP 4
#define TRK_LOOP_ON 5
#define TRK_LOOP_OFF 6
#define TRK_LOAD 7

#define RSP_VERSION_STRING 129
#define RSP_SYSTEM_INFO 130
#define RSP_STATUS 131
#define RSP_TRACK_REPORT 132

#define MAX_MESSAGE_LEN 32
#define MAX_NUM_VOICES 14
#define VERSION_STRING_LEN 21

#define SOM1 0xf0
#define SOM2 0xaa
#define EOM 0x55

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)

#if (RPU_OS_HARDWARE_REV <= 3)
#define WTSerial Serial
#else
#define WTSerial Serial1
#endif

class wavTrigger {
 public:
   wavTrigger() {
      ;
   }
   virtual ~wavTrigger() {
      ;
   }

   void start(void);
   void update(void);
   void flush(void);
   void setReporting(bool enable);
   void setAmpPwr(bool enable);
   bool getVersion(char* pDst, int len);
   int getNumTracks(void);
   bool isTrackPlaying(int trk);
   int getPlayingTrack(int voiceNum);
   void masterGain(int gain);
   void stopAllTracks(void);
   void resumeAllInSync(void);
   void trackPlaySolo(int trk);
   void trackPlaySolo(int trk, bool lock);
   void trackPlayPoly(int trk);
   void trackPlayPoly(int trk, bool lock);
   void trackLoad(int trk);
   void trackLoad(int trk, bool lock);
   void trackStop(int trk);
   void trackPause(int trk);
   void trackResume(int trk);
   void trackLoop(int trk, bool enable);
   void trackGain(int trk, int gain);
   void trackFade(int trk, int gain, int time, bool stopFlag);
   void samplerateOffset(int offset);
   void setTriggerBank(int bank);

 private:
   void trackControl(int trk, int code);
   void trackControl(int trk, int code, bool lock);

   uint16_t voiceTable[MAX_NUM_VOICES];
   uint8_t rxMessage[MAX_MESSAGE_LEN];
   char version[VERSION_STRING_LEN];
   uint16_t numTracks;
   uint8_t numVoices;
   uint8_t rxCount;
   uint8_t rxLen;
   bool rxMsgReady;
   bool versionRcvd;
   bool sysinfoRcvd;
};

#endif

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

   bool PlayBackgroundSoundtrack(AudioSoundtrack* soundtrackArray, unsigned short numSoundtrackEntries, unsigned long currentTime,
                                 bool randomOrder = true);
   bool PlayBackgroundSong(unsigned short trackIndex, bool loopTrack = true);

   bool PlaySound(unsigned short soundIndex, uint8_t audioType, uint8_t overrideVolume = 0xFF);
   bool FadeSound(unsigned short soundIndex, int fadeGain, int numMilliseconds, bool stopTrack);

   bool QueueSound(unsigned short soundIndex, uint8_t audioType, unsigned long timeToPlay, uint8_t overrideVolume = 0xFF);
   inline bool QueueOriginalSound(unsigned short soundIndex, unsigned long timeToPlay, uint8_t overrideVolume = 0xFF) {
      return QueueSound(soundIndex, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, timeToPlay, overrideVolume);
   }
   inline bool QueueWavTriggerSound(unsigned short soundIndex, unsigned long timeToPlay, uint8_t overrideVolume = 0xFF) {
      return QueueSound(soundIndex, AUDIO_PLAY_TYPE_WAV_TRIGGER, timeToPlay, overrideVolume);
   }

   bool QueueSoundCardCommand(uint8_t scFunction, uint8_t scRegister, uint8_t scData, unsigned long startTime);

   bool PlaySoundCardWhenPossible(unsigned short soundEffectNum, unsigned long currentTime, unsigned long requestedPlayTime = 0,
                                  unsigned long playUntil = 50, uint8_t priority = 10);

   bool QueuePrioritizedNotification(unsigned short notificationIndex, unsigned short notificationLength, uint8_t priority,
                                     unsigned long currentTime);

   bool Update(unsigned long currentTime);

   bool StopSound(unsigned short soundIndex);
   bool StopCurrentNotification(uint8_t priority = 10);
   bool StopAllMusic();
   bool StopAllNotifications(uint8_t priority = 10);
   bool StopAllSoundFX();
   bool StopAllAudio();

 private:
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
   unsigned short curSoundtrackEntries;
   unsigned short currentBackgroundTrack;

   SoundEntry soundQueue[SOUND_QUEUE_SIZE];

#if defined(RPU_OS_USE_WTYPE_1_SOUND) || defined(RPU_OS_USE_WTYPE_2_SOUND)
   SoundEffectEntry CurrentSoundPlaying;
   SoundEffectEntry SoundEffectQueue[SOUND_EFFECT_QUEUE_SIZE];
#endif

#ifdef RPU_OS_USE_SB300
   SoundCardCommandEntry soundCardQueue[SOUND_CARD_QUEUE_SIZE];
#endif

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
   wavTrigger wTrig; // Our WAV Trigger object
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

   unsigned long nextVoiceNotificationPlayTime;
   unsigned long backgroundSongEndTime;
};

#define AUDIO_HANDLER_H
#endif
