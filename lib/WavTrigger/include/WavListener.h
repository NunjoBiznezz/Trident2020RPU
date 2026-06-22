

#pragma once

class WavListener {
public:
    virtual ~WavListener() {}

    /**
     * \brief Callback function invoked when the Wav Trigger version string is received.
     * \param version a pointer to the version string (const char*)
     */
      virtual void onVersionReceived(const char* version) {
          // Default implementation does nothing
      }

      /**
       * \brief Callback function invoked when a track report is received from the Wav Trigger.
       * \param trackNum the track number being reported (uint16_t)
       * \param voiceNumber the voice number associated with the track (uint8_t)
       * \param isPlaying true if the track is playing, false if stopped (bool)
       */
      virtual void onTrackReport(uint16_t trackNum, uint8_t voiceNumber, bool isPlaying) {
          // Default implementation does nothing
      }

      /**
       *
       * \brief Callback function invoked when system information is received from the Wav Trigger.
       * \param numVoices the number of voices supported by the Wav Trigger (uint8_t)
       * \param numTracks the total number of tracks available on the Wav Trigger (uint16_t)
       */
      virtual void onSystemInfoReceived(uint8_t numVoices, uint16_t numTracks) {
          // Default implementation does nothing
      }
};
