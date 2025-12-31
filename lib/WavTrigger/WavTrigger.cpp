#include "WavTrigger.h"
#include <Arduino.h>

constexpr uint8_t CMD_GET_VERSION = 1;
constexpr uint8_t CMD_GET_SYS_INFO = 2;
constexpr uint8_t CMD_TRACK_CONTROL = 3;
constexpr uint8_t CMD_STOP_ALL = 4;
constexpr uint8_t CMD_MASTER_VOLUME = 5;
constexpr uint8_t CMD_TRACK_VOLUME = 8;
constexpr uint8_t CMD_AMP_POWER = 9;
constexpr uint8_t CMD_TRACK_FADE = 10;
constexpr uint8_t CMD_RESUME_ALL_SYNC = 11;
constexpr uint8_t CMD_SAMPLERATE_OFFSET = 12;
constexpr uint8_t CMD_TRACK_CONTROL_EX = 13;
constexpr uint8_t CMD_SET_REPORTING = 14;
constexpr uint8_t CMD_SET_TRIGGER_BANK = 15;

constexpr uint8_t TRK_PLAY_SOLO = 0;
constexpr uint8_t TRK_PLAY_POLY = 1;
constexpr uint8_t TRK_PAUSE = 2;
constexpr uint8_t TRK_RESUME = 3;
constexpr uint8_t TRK_STOP = 4;
constexpr uint8_t TRK_LOOP_ON = 5;
constexpr uint8_t TRK_LOOP_OFF = 6;
constexpr uint8_t TRK_LOAD = 7;

constexpr uint8_t RSP_VERSION_STRING = 129;
constexpr uint8_t RSP_SYSTEM_INFO = 130;
constexpr uint8_t RSP_STATUS = 131;
constexpr uint8_t RSP_TRACK_REPORT = 132;

constexpr uint8_t SOM1 = 0xf0;
constexpr uint8_t SOM2 = 0xaa;
constexpr uint8_t EOM = 0x55;

#if (RPU_OS_HARDWARE_REV <= 3)
#define WTSerial Serial
#else
#define WTSerial Serial1    // Hardware serial
#endif

// **************************************************************
void WavTrigger::start(void) {
   uint8_t txbuf[5];

   versionRcvd = false;
   sysinfoRcvd = false;
   WTSerial.begin(57600);
   flush();

   // Request version string
   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x05;
   txbuf[3] = CMD_GET_VERSION;
   txbuf[4] = EOM;
   WTSerial.write(txbuf, 5);

   // Request system info
   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x05;
   txbuf[3] = CMD_GET_SYS_INFO;
   txbuf[4] = EOM;
   WTSerial.write(txbuf, 5);
}

// **************************************************************
void WavTrigger::flush(void) {
   int i;

   rxCount = 0;
   rxLen = 0;
   rxMsgReady = false;
   for (i = 0; i < MAX_NUM_VOICES; i++) {
      voiceTable[i] = 0xffff;
   }
   while (WTSerial.available()) {
      WTSerial.read();
   }
}

// **************************************************************
void WavTrigger::update(void) {
   if (RPU_OS_HARDWARE_REV <= 3) {
      return;
   }

   int i;
   uint8_t dat;
   uint8_t voice;
   uint16_t track;

   rxMsgReady = false;
   while (WTSerial.available() > 0) {
      dat = WTSerial.read();
      if ((rxCount == 0) && (dat == SOM1)) {
         rxCount++;
      } else if (rxCount == 1) {
         if (dat == SOM2) {
            rxCount++;
         } else {
            rxCount = 0;
         }
      } else if (rxCount == 2) {
         if (dat <= MAX_MESSAGE_LEN) {
            rxCount++;
            rxLen = dat - 1;
         } else {
            rxCount = 0;
         }
      } else if ((rxCount > 2) && (rxCount < rxLen)) {
         rxMessage[rxCount - 3] = dat;
         rxCount++;
      } else if (rxCount == rxLen) {
         if (dat == EOM) {
            rxMsgReady = true;
         } else {
            rxCount = 0;
         }
      } else {
         rxCount = 0;
      }

      if (rxMsgReady) {
         switch (rxMessage[0]) {
         case RSP_TRACK_REPORT:
            track = rxMessage[2];
            track = (track << 8) + rxMessage[1] + 1;
            voice = rxMessage[3];
            if (voice < MAX_NUM_VOICES) {
               if (rxMessage[4] == 0) {
                  if (track == voiceTable[voice]) {
                     voiceTable[voice] = 0xffff;
                  }
               } else {
                  voiceTable[voice] = track;
               }
            }
            break;

         case RSP_VERSION_STRING:
            for (i = 0; i < (VERSION_STRING_LEN - 1); i++) {
               version[i] = rxMessage[i + 1];
            }
            version[VERSION_STRING_LEN - 1] = 0;
            versionRcvd = true;
            break;

         case RSP_SYSTEM_INFO:
            numVoices = rxMessage[1];
            numTracks = rxMessage[3];
            numTracks = (numTracks << 8) + rxMessage[2];
            sysinfoRcvd = true;
            break;
         }
         rxCount = 0;
         rxLen = 0;
         rxMsgReady = false;
      }
   }
}

// **************************************************************
bool WavTrigger::isTrackPlaying(int trk) {
   int i;
   bool fResult = false;

   update();
   for (i = 0; i < MAX_NUM_VOICES; i++) {
      if (voiceTable[i] == ((uint16_t)trk)) {
         fResult = true;
      }
   }

   return fResult;
}

int WavTrigger::getPlayingTrack(int voiceNum) {
   if (voiceNum >= MAX_NUM_VOICES || voiceNum < 0) {
      return 0xFFFF;
   }
   return (voiceTable[voiceNum]);
}

// **************************************************************
void WavTrigger::masterGain(int gain) {
   uint8_t txbuf[7];
   uint16_t vol;

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x07;
   txbuf[3] = CMD_MASTER_VOLUME;
   vol = (uint16_t)gain;
   txbuf[4] = (uint8_t)vol;
   txbuf[5] = (uint8_t)(vol >> 8);
   txbuf[6] = EOM;
   WTSerial.write(txbuf, 7);
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
   WTSerial.write(txbuf, 6);
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
   WTSerial.write(txbuf, 6);
}

// **************************************************************
bool WavTrigger::getVersion(char* pDst, int len) {
   int i;

   update();
   if (!versionRcvd) {
      return false;
   }
   for (i = 0; i < (VERSION_STRING_LEN - 1); i++) {
      if (i >= (len - 1)) {
         break;
      }
      pDst[i] = version[i];
   }
   pDst[++i] = 0;
   return true;
}

// **************************************************************
int WavTrigger::getNumTracks(void) {
   update();
   return numTracks;
}

// **************************************************************
void WavTrigger::trackPlaySolo(int trk) {
   trackControl(trk, TRK_PLAY_SOLO);
}

// **************************************************************
void WavTrigger::trackPlaySolo(int trk, bool lock) {
   trackControl(trk, TRK_PLAY_SOLO, lock);
}

// **************************************************************
void WavTrigger::trackPlayPoly(int trk) {
   trackControl(trk, TRK_PLAY_POLY);
}

// **************************************************************
void WavTrigger::trackPlayPoly(int trk, bool lock) {
   trackControl(trk, TRK_PLAY_POLY, lock);
}

// **************************************************************
void WavTrigger::trackLoad(int trk) {
   trackControl(trk, TRK_LOAD);
}

// **************************************************************
void WavTrigger::trackLoad(int trk, bool lock) {
   trackControl(trk, TRK_LOAD, lock);
}

// **************************************************************
void WavTrigger::trackStop(int trk) {
   trackControl(trk, TRK_STOP);
}

// **************************************************************
void WavTrigger::trackPause(int trk) {
   trackControl(trk, TRK_PAUSE);
}

// **************************************************************
void WavTrigger::trackResume(int trk) {
   trackControl(trk, TRK_RESUME);
}

// **************************************************************
void WavTrigger::trackLoop(int trk, bool enable) {
   if (enable) {
      trackControl(trk, TRK_LOOP_ON);
   } else {
      trackControl(trk, TRK_LOOP_OFF);
   }
}

// **************************************************************
void WavTrigger::trackControl(int trk, uint8_t code) {
   uint8_t txbuf[8];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x08;
   txbuf[3] = CMD_TRACK_CONTROL;
   txbuf[4] = (uint8_t)code;
   txbuf[5] = (uint8_t)trk;
   txbuf[6] = (uint8_t)(trk >> 8);
   txbuf[7] = EOM;
   WTSerial.write(txbuf, 8);
}

// **************************************************************
void WavTrigger::trackControl(int trk, uint8_t code, bool lock) {
   uint8_t txbuf[9];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x09;
   txbuf[3] = CMD_TRACK_CONTROL_EX;
   txbuf[4] = (uint8_t)code;
   txbuf[5] = (uint8_t)trk;
   txbuf[6] = (uint8_t)(trk >> 8);
   txbuf[7] = lock;
   txbuf[8] = EOM;
   WTSerial.write(txbuf, 9);
}

// **************************************************************
void WavTrigger::stopAllTracks(void) {
   uint8_t txbuf[5];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x05;
   txbuf[3] = CMD_STOP_ALL;
   txbuf[4] = EOM;
   WTSerial.write(txbuf, 5);
}

// **************************************************************
void WavTrigger::resumeAllInSync(void) {
   uint8_t txbuf[5];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x05;
   txbuf[3] = CMD_RESUME_ALL_SYNC;
   txbuf[4] = EOM;
   WTSerial.write(txbuf, 5);
}

// **************************************************************
void WavTrigger::trackGain(int trk, int gain) {
   uint8_t txbuf[9];
   uint16_t vol;

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x09;
   txbuf[3] = CMD_TRACK_VOLUME;
   txbuf[4] = (uint8_t)trk;
   txbuf[5] = (uint8_t)(trk >> 8);
   vol = (uint16_t)gain;
   txbuf[6] = (uint8_t)vol;
   txbuf[7] = (uint8_t)(vol >> 8);
   txbuf[8] = EOM;
   WTSerial.write(txbuf, 9);
}

// **************************************************************
void WavTrigger::trackFade(int trk, int gain, int time, bool stopFlag) {
   uint8_t txbuf[12];
   uint16_t vol;

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x0c;
   txbuf[3] = CMD_TRACK_FADE;
   txbuf[4] = (uint8_t)trk;
   txbuf[5] = (uint8_t)(trk >> 8);
   vol = (uint16_t)gain;
   txbuf[6] = (uint8_t)vol;
   txbuf[7] = (uint8_t)(vol >> 8);
   txbuf[8] = (uint8_t)time;
   txbuf[9] = (uint8_t)(time >> 8);
   txbuf[10] = stopFlag;
   txbuf[11] = EOM;
   WTSerial.write(txbuf, 12);
}

// **************************************************************
void WavTrigger::samplerateOffset(int offset) {
   uint8_t txbuf[7];
   uint16_t off;

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x07;
   txbuf[3] = CMD_SAMPLERATE_OFFSET;
   off = (uint16_t)offset;
   txbuf[4] = (uint8_t)off;
   txbuf[5] = (uint8_t)(off >> 8);
   txbuf[6] = EOM;
   WTSerial.write(txbuf, 7);
}

// **************************************************************
void WavTrigger::setTriggerBank(int bank) {
   uint8_t txbuf[6];

   txbuf[0] = SOM1;
   txbuf[1] = SOM2;
   txbuf[2] = 0x06;
   txbuf[3] = CMD_SET_TRIGGER_BANK;
   txbuf[4] = (uint8_t)bank;
   txbuf[5] = EOM;
   WTSerial.write(txbuf, 6);
}

