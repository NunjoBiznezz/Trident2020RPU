#ifndef WAV_TRIGGER_H
#define WAV_TRIGGER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <stdint.h>

class WavListener;

constexpr uint16_t EMPTY_SLOT = 0xFFFF;

class WavTrigger {
 public:
   WavTrigger(HardwareSerial& serial) : voiceTable_{}, rxMessage_{}, version_{}, numTracks_(0), numVoices_(0),
                                        rxCount_(0), rxLen_(0),
                                        rxMsgReady_(false),
                                        versionRcvd_(false),
                                        sysinfoRcvd_(false),
                                        serial_(serial) {
   }

   virtual ~WavTrigger() {}

   void start(void);
   void update(void);
   void flush(void);

   /**
    * \brief Enables or disables reporting of track status and system information. When enabled, the WavTrigger will call the appropriate
    * callback functions in the WavListener when it receives updates from the Wav Trigger.
    * \param enable true to enable reporting, false to disable it (bool)
    */
   void setReporting(bool enable);

   /**
    * \brief Turns the on-board amplifier on or off.
    * \param enable true to turn on the amplifier, false to turn it off (bool
    */
   void setAmpPwr(bool enable);

   /**
    * Request version information from the Wav Trigger. The version string will be returned via the onVersionReceived callback.
    * \param pDst a pointer to a character array where the version string will be stored (char*)
    * \param len the length of the character array (int)
    * \return true if the version string was successfully retrieved, false otherwise (bool)
    */
   bool getVersion(char* pDst, int len);

   int getNumTracks(void);
   bool isTrackPlaying(uint16_t trk);
   uint16_t getPlayingTrack(int voiceNum) const;

   /**
    * \brief Updates the output volume of the WAV Trigger with the specified gain. Gain value is a signed 16-
bit integer, -70dB to +10dB
    */
   void masterGain(int16_t gain);

   void stopAllTracks(void);
   void resumeAllInSync(void);

   /**
    * \brief Play track without polyphony, stops all other tracks
    * \param trk the track number to play (uint16_t)
    */
   void trackPlaySolo(uint16_t trk);
   void trackPlaySolo(uint16_t trk, bool lock);

   /**
    * \brief Play track with polyphony, does not stop other tracks
    * \param trk the track number to play (uint16_t)
    */
   void trackPlayPoly(uint16_t trk);
   void trackPlayPoly(uint16_t trk, bool lock);

   /**
    * \brief Load and pause track
    * \param trk the track number to load (uint16_t)
    */
   void trackLoad(uint16_t trk);
   void trackLoad(uint16_t trk, bool lock);

   /**
    * \brief Stop the specified track
    * \param trk the track number to stop (uint16_t)
    */
   void trackStop(uint16_t trk);

   /**
    * \brief Pause the specified track
    * \param trk the track number to pause (uint16_t)
    */
   void trackPause(uint16_t trk);

   /**
    * \brief Resume the specified track
    * \param trk the track number to resume (uint16_t)
    */
   void trackResume(uint16_t trk);

   /**
    * \brief Enables or disables looping for the specified track.
    * \param trk the track number to set looping for (uint16_t)
    * \param enable true to enable looping, false to disable it (bool)
    */
   void trackLoop(uint16_t trk, bool enable);

   /**
    * \brief Updates the gain of a specific track with the specified gain. Gain value is a
    * signed 16-bit integer, -70dB to +10dB. The new gain becomes effective immediately
    * \param trk the track number to set the gain for (uint16_t)
    * \param gain the gain value -70 to +10 dB (int)
    */
   void trackGain(uint16_t trk, int16_t gain);

   /**
    * \brief Fades the track to the specified gain over the specified time. If stopFlag is true, the track will be stopped at the end of the
    * fade.
    * \param trk the track number to fade (uint16_t)
    * \param gain the target gain -70 to +10 dB
    * \param time the fade time in milliseconds (uint16_t)
    * \param stopFlag if true, the track will be stopped at the end of the fade (bool)
    */
   void trackFade(uint16_t trk, int16_t gain, int time, bool stopFlag);

   /**
    * \brief Increases or decreases the WAV Trigger’s playback speed for all tracks. Offset value is a signed
    * 16-bit integer, -32767 to +32767.
    * \param offset the sample rate offset value
    * \example  0x1f5a Increases pitch 2 semitones.
    */
   void samplerateOffset(int offset);

   /**
    * \brief Sets the trigger bank to the specified value. (1-32).
    * \param bank the trigger bank number (int)
    */
   void setTriggerBank(uint8_t bank);

   /**
    * \brief Returns the maximum number of voices supported by the Wav Trigger.
    * \return the maximum number of voices (int)
    */
   static int maxNumVoices() {
      return MAX_NUM_VOICES;
   }

   /**
    * \brief Sets the listener object that will receive callbacks for Wav Trigger events.
    * \param newListener a pointer to the WavListener object (WavListener*)
    */
   void setListener(WavListener* newListener) {
      listener_ = newListener;
   }

   // Rev 3 board lacks an RX line; can be overridden by soldering a jumper and defining WAVTRIGGER_LACKS_RX
#if defined(WAVTRIGGER_LACKS_RX)
   static constexpr bool hasSerialRx = false;
#else
   static constexpr bool hasSerialRx = true;
#endif

 public:
   static constexpr int MAX_NUM_VOICES = 14;
   static constexpr int8_t UNITY_GAIN = 0;
   static constexpr int8_t MAX_GAIN = 10;   // +10 dB
   static constexpr int8_t MIN_GAIN = -70;  // -70 dB

   uint8_t getNumVoices() const {
      return numVoices_;
   }

   uint8_t getNumTracks() const {
      return numTracks_;
   }

 protected:
   void requestVersion() const;
   void requestSystemInfo() const;

   static constexpr int MAX_MESSAGE_LEN = 32;
   static constexpr int VERSION_STRING_LEN = 21;

   void trackControl(uint16_t trk, uint8_t code);
   void trackControl(uint16_t trk, uint8_t code, bool lock);

   uint16_t voiceTable_[MAX_NUM_VOICES];
   uint8_t rxMessage_[MAX_MESSAGE_LEN];
   char version_[VERSION_STRING_LEN];
   uint16_t numTracks_;
   uint8_t numVoices_;
   uint8_t rxCount_;
   uint8_t rxLen_;
   bool rxMsgReady_;
   bool versionRcvd_;
   bool sysinfoRcvd_;
   WavListener* listener_ = nullptr;
   HardwareSerial& serial_;
};

#endif // WAV_TRIGGER_H
