#include "WavTrigger.h"
#include "WavListener.h"
#include <Arduino.h>

static constexpr uint8_t CMD_GET_VERSION = 1;
static constexpr uint8_t CMD_GET_SYS_INFO = 2;
static constexpr uint8_t CMD_TRACK_CONTROL = 3;
static constexpr uint8_t CMD_STOP_ALL = 4;
static constexpr uint8_t CMD_MASTER_VOLUME = 5;
static constexpr uint8_t CMD_TRACK_VOLUME = 8;
static constexpr uint8_t CMD_AMP_POWER = 9;
static constexpr uint8_t CMD_TRACK_FADE = 10;
static constexpr uint8_t CMD_RESUME_ALL_SYNC = 11;
static constexpr uint8_t CMD_SAMPLERATE_OFFSET = 12;
static constexpr uint8_t CMD_TRACK_CONTROL_EX = 13;
static constexpr uint8_t CMD_SET_REPORTING = 14;
static constexpr uint8_t CMD_SET_TRIGGER_BANK = 15;

static constexpr int TRK_PLAY_SOLO = 0;
static constexpr int TRK_PLAY_POLY = 1;
static constexpr int TRK_PAUSE = 2;
static constexpr int TRK_RESUME = 3;
static constexpr int TRK_STOP = 4;
static constexpr int TRK_LOOP_ON = 5;
static constexpr int TRK_LOOP_OFF = 6;
static constexpr int TRK_LOAD = 7;

static constexpr uint8_t RSP_VERSION_STRING = 129;
static constexpr uint8_t RSP_SYSTEM_INFO = 130;
static constexpr uint8_t RSP_STATUS = 131;
static constexpr uint8_t RSP_TRACK_REPORT = 132;

static constexpr uint8_t SOM1 = 0xf0;
static constexpr uint8_t SOM2 = 0xaa;
static constexpr uint8_t EOM = 0x55;

// **************************************************************
void WavTrigger::start(void) {
   versionRcvd_ = false;
   sysinfoRcvd_ = false;
   flush();

   requestVersion();
   requestSystemInfo();
}

void WavTrigger::requestVersion() const {
   uint8_t txbuf[5];

   // Request version string
   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x05;
   txbuf[3] = CMD_GET_VERSION;
   txbuf[4] = EOM;
   serial_.write(txbuf, 5);
}

void WavTrigger::requestSystemInfo() const {
   uint8_t txbuf[5];

   // Request system info
   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x05;
   txbuf[3] = CMD_GET_SYS_INFO;
   txbuf[4] = EOM;
   serial_.write(txbuf, 5);
}

// **************************************************************
void WavTrigger::flush(void) {
   // uint8_t dat;

   rxCount_ = 0;
   rxLen_ = 0;
   rxMsgReady_ = false;
   for (int i = 0; i < MAX_NUM_VOICES; i++) {
      voiceTable_[i] = EMPTY_SLOT;
   }
   while (serial_.available()) {
      /*dat = */ serial_.read();
   }
}

// **************************************************************
void WavTrigger::update(void) {

   // Not gonna work unless an RX is wired in. Rev 3 and below do not have it
   // BUT you can jumper onto the Rev3
   if (!hasSerialRx) {
      return;
   }

   rxMsgReady_ = false;
   while (serial_.available() > 0) {
      const uint8_t dat = serial_.read();
      if ((this->rxCount_ == 0) && (dat == SOM1)) {
         this->rxCount_++;
      } else if (this->rxCount_ == 1) {
         if (dat == SOM2) {
            this->rxCount_++;
         } else {
            this->rxCount_ = 0;
            // Serial.print("Bad msg 1\n");
         }
      } else if (this->rxCount_ == 2) {
         if (dat <= MAX_MESSAGE_LEN) {
            this->rxCount_++;
            this->rxLen_ = dat - 1;
         } else {
            this->rxCount_ = 0;
            // Serial.print("Bad msg 2\n");
         }
      } else if ((this->rxCount_ > 2) && (this->rxCount_ < this->rxLen_)) {
         this->rxMessage_[this->rxCount_ - 3] = dat;
         this->rxCount_++;
      } else if (this->rxCount_ == this->rxLen_) {
         if (dat == EOM) {
            this->rxMsgReady_ = true;
         } else {
            this->rxCount_ = 0;
            // Serial.print("Bad msg 3\n");
         }
      } else {
         rxCount_ = 0;
         // Serial.print("Bad msg 4\n");
      }

      if (rxMsgReady_) {
         switch (rxMessage_[0]) {
         case RSP_TRACK_REPORT: {
            uint16_t track = (static_cast<uint16_t>(rxMessage_[2]) << 8) + static_cast<uint16_t>(rxMessage_[1]) + 1;
            uint8_t voice = rxMessage_[3];
            bool isPlaying = (rxMessage_[4] != 0);
            if (voice < MAX_NUM_VOICES) {
               if (rxMessage_[4] == 0) {
                  if (track == voiceTable_[voice]) {
                     voiceTable_[voice] = EMPTY_SLOT;
                  }
               } else {
                  voiceTable_[voice] = track;
               }
            }

            if (listener_) {
               // int16_t gain = static_cast<int16_t>(rxMessage[5]);
               // bool isLooping = (rxMessage[6] != 0);

               listener_->onTrackReport(track, voice, isPlaying);
            }
            // ==========================
            // Serial.print("Track ");
            // Serial.print(track);
            // if (rxMessage[4] == 0)
            //  Serial.print(" off\n");
            // else
            //  Serial.print(" on\n");
            // ==========================
            break;
         }

         case RSP_VERSION_STRING:
            for (int i = 0; i < (VERSION_STRING_LEN - 1); i++) {
               this->version_[i] = static_cast<char>(rxMessage_[i + 1]);
            }
            this->version_[VERSION_STRING_LEN - 1] = 0;
            this->versionRcvd_ = true;

            if (this->listener_) {
               this->listener_->onVersionReceived(this->version_);
            }

            // ==========================
            // Serial.write(version);
            // Serial.write("\n");
            // ==========================
            break;

         case RSP_SYSTEM_INFO:
            this->numVoices_ = rxMessage_[1];
            this->numTracks_ = rxMessage_[3];
            this->numTracks_ = (this->numTracks_ << 8) + rxMessage_[2];
            this->sysinfoRcvd_ = true;

            if (this->listener_) {
               this->listener_->onSystemInfoReceived(this->numVoices_, this->numTracks_);
            }
            // ==========================
            ///\Serial.print("Sys info received\n");
            // ==========================
            break;

         // Too much data received at one time, commented out for now
         case RSP_STATUS:
            break;

         default:
               break;
         }

         this->rxCount_ = 0;
         this->rxLen_ = 0;
         this->rxMsgReady_ = false;

      } // if (rxMsgReady_)

   } // while (serial_.available() > 0)
}

// **************************************************************
bool WavTrigger::isTrackPlaying(uint16_t trk) {
   bool fResult = false;

   update();
   for (int i = 0; i < MAX_NUM_VOICES; i++) {
      if (voiceTable_[i] == ((uint16_t)trk)) {
         fResult = true;
      }
   }

   return fResult;
}

uint16_t WavTrigger::getPlayingTrack(int voiceNum) const {
   if (voiceNum >= MAX_NUM_VOICES || voiceNum < 0) {
      return EMPTY_SLOT;
   }
   return (voiceTable_[voiceNum]);
}

// **************************************************************
void WavTrigger::masterGain(int16_t gain) {
   uint8_t txbuf[7];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x07;
   txbuf[3] = CMD_MASTER_VOLUME;
   uint16_t vol = static_cast<uint16_t>(gain);
   txbuf[4] = static_cast<uint8_t>(vol);
   txbuf[5] = static_cast<uint8_t>(vol >> 8);
   txbuf[6] = EOM;
   serial_.write(txbuf, 7);
}

// **************************************************************
void WavTrigger::setAmpPwr(bool enable) {
   uint8_t txbuf[6];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x06;
   txbuf[3] = CMD_AMP_POWER;
   txbuf[4] = enable;
   txbuf[5] = EOM;
   serial_.write(txbuf, 6);
}

// **************************************************************
void WavTrigger::setReporting(bool enable) {
   uint8_t txbuf[6];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x06;
   txbuf[3] = CMD_SET_REPORTING;
   txbuf[4] = enable;
   txbuf[5] = EOM;
   serial_.write(txbuf, 6);
}

// **************************************************************
bool WavTrigger::getVersion(char* pDst, int len) {
   int i;

   update();
   if (!versionRcvd_) {
      return false;
   }
   for (i = 0; i < (VERSION_STRING_LEN - 1); i++) {
      if (i >= (len - 1)) {
         break;
      }
      pDst[i] = version_[i];
   }
   pDst[++i] = 0;
   return true;
}

// **************************************************************
int WavTrigger::getNumTracks(void) {
   update();
   return static_cast<int>(numTracks_);
}

// **************************************************************
void WavTrigger::trackPlaySolo(uint16_t trk) {
   trackControl(trk, TRK_PLAY_SOLO);
}

// **************************************************************
void WavTrigger::trackPlaySolo(uint16_t trk, bool lock) {
   trackControl(trk, TRK_PLAY_SOLO, lock);
}

// **************************************************************
void WavTrigger::trackPlayPoly(uint16_t trk) {
   trackControl(trk, TRK_PLAY_POLY);
}

// **************************************************************
void WavTrigger::trackPlayPoly(uint16_t trk, bool lock) {
   trackControl(trk, TRK_PLAY_POLY, lock);
}

// **************************************************************
void WavTrigger::trackLoad(uint16_t trk) {
   trackControl(trk, TRK_LOAD);
}

// **************************************************************
void WavTrigger::trackLoad(uint16_t trk, bool lock) {
   trackControl(trk, TRK_LOAD, lock);
}

// **************************************************************
void WavTrigger::trackStop(uint16_t trk) {
   trackControl(trk, TRK_STOP);
}

// **************************************************************
void WavTrigger::trackPause(uint16_t trk) {
   trackControl(trk, TRK_PAUSE);
}

// **************************************************************
void WavTrigger::trackResume(uint16_t trk) {
   trackControl(trk, TRK_RESUME);
}

// **************************************************************
void WavTrigger::trackLoop(uint16_t trk, bool enable) {
   if (enable) {
      trackControl(trk, TRK_LOOP_ON);
   } else {
      trackControl(trk, TRK_LOOP_OFF);
   }
}

// **************************************************************
void WavTrigger::trackControl(uint16_t trk, uint8_t code) {
   uint8_t txbuf[8];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x08;
   txbuf[3] = CMD_TRACK_CONTROL;
   txbuf[4] = code;
   txbuf[5] = static_cast<uint8_t>(trk);
   txbuf[6] = static_cast<uint8_t>(trk >> 8);
   txbuf[7] = EOM;
   serial_.write(txbuf, 8);
}

// **************************************************************
void WavTrigger::trackControl(uint16_t trk, uint8_t code, bool lock) {
   uint8_t txbuf[9];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x09;
   txbuf[3] = CMD_TRACK_CONTROL_EX;
   txbuf[4] = code;
   txbuf[5] = static_cast<uint8_t>(trk);
   txbuf[6] = static_cast<uint8_t>(trk >> 8);
   txbuf[7] = lock;
   txbuf[8] = EOM;
   serial_.write(txbuf, 9);
}

// **************************************************************
void WavTrigger::stopAllTracks(void) {
   uint8_t txbuf[5];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x05;
   txbuf[3] = CMD_STOP_ALL;
   txbuf[4] = EOM;
   serial_.write(txbuf, 5);
}

// **************************************************************
void WavTrigger::resumeAllInSync(void) {
   uint8_t txbuf[5];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x05;
   txbuf[3] = CMD_RESUME_ALL_SYNC;
   txbuf[4] = EOM;
   serial_.write(txbuf, 5);
}

// **************************************************************
void WavTrigger::trackGain(uint16_t trk, int16_t gain) {
   uint8_t txbuf[9];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x09;
   txbuf[3] = CMD_TRACK_VOLUME;
   txbuf[4] = static_cast<uint8_t>(trk);
   txbuf[5] = static_cast<uint8_t>(trk >> 8);
   const uint16_t vol = static_cast<uint16_t>(gain);
   txbuf[6] = static_cast<uint8_t>(vol);
   txbuf[7] = static_cast<uint8_t>(vol >> 8);
   txbuf[8] = EOM;
   serial_.write(txbuf, 9);
}

// **************************************************************
void WavTrigger::trackFade(uint16_t trk, int16_t gain, int time, bool stopFlag) {
   uint8_t txbuf[12];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x0c;
   txbuf[3] = CMD_TRACK_FADE;
   txbuf[4] = static_cast<uint8_t>(trk);
   txbuf[5] = static_cast<uint8_t>(trk >> 8);
   uint16_t vol = static_cast<uint16_t>(gain);
   txbuf[6] = static_cast<uint8_t>(vol);
   txbuf[7] = static_cast<uint8_t>(vol >> 8);
   txbuf[8] = static_cast<uint8_t>(time);
   txbuf[9] = static_cast<uint8_t>(time >> 8);
   txbuf[10] = stopFlag;
   txbuf[11] = EOM;
   serial_.write(txbuf, 12);
}

// **************************************************************
void WavTrigger::samplerateOffset(int offset) {
   uint8_t txbuf[7];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x07;
   txbuf[3] = CMD_SAMPLERATE_OFFSET;
   uint16_t off = static_cast<uint16_t>(offset);
   txbuf[4] = static_cast<uint8_t>(off);
   txbuf[5] = static_cast<uint8_t>(off >> 8);
   txbuf[6] = EOM;
   serial_.write(txbuf, 7);
}

// **************************************************************
void WavTrigger::setTriggerBank(uint8_t bank) {
   uint8_t txbuf[6];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x06;
   txbuf[3] = CMD_SET_TRIGGER_BANK;
   txbuf[4] = bank;
   txbuf[5] = EOM;
   serial_.write(txbuf, 6);
}

