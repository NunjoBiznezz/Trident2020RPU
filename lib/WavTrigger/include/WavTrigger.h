#ifndef WAV_TRIGGER_H
#define WAV_TRIGGER_H

#include <HardwareSerial.h>
#include <stdint.h>


class WavTrigger {
 public:
   WavTrigger() {}
   virtual ~WavTrigger() {}

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

   static int maxNumVoices() {
      return MAX_NUM_VOICES;
   }

 private:
   static constexpr int MAX_MESSAGE_LEN = 32;
   static constexpr int MAX_NUM_VOICES = 14;
   static constexpr int VERSION_STRING_LEN = 21;

   void trackControl(int trk, uint8_t code);
   void trackControl(int trk, uint8_t code, bool lock);

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

#endif // WAV_TRIGGER_H
