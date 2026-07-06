/**************************************************************************
 * TridentMachine.h
 *
 * Concrete PinballMachine implementation for the Trident hardware.
 * Owns the audio subsystem and the single MachineSettings instance.
 * Constructed once in main.cpp; other objects hold a PinballMachine*
 * for audio/credit calls and a MachineSettings* for direct field access.
 **************************************************************************/

#pragma once
#include "MachineSettings.h"
#include "PinballMachine.h"
#include "AudioHandler.h"
#if defined(RPU_OS_USE_WAV_TRIGGER)
#include "WavTriggerHandler.h"
#endif
#include <stdint.h>

class TridentMachine : public PinballMachine {
public:
   TridentMachine() = default;

   MachineSettings& settings()            { return settings_; }

   // One-time hardware init; call from setup() before the main loop.
   void init(unsigned long currentTime);

   // Queue a WAV notification during boot diagnostics.
   void queueDiagNotification(unsigned short notificationNum, unsigned long currentTime);

   // PinballMachine interface — see PinballMachine.h for semantics.
   void playSoundEffect(uint8_t soundEffectNum) override;
   void playBackgroundSong(unsigned short songNum) override;
   void playBackgroundSongBasedOnBall(uint8_t ballNum) override;
   void addCredit(bool playSound, uint8_t numToAdd) override;
   void addSpecialCredit() override;
   void addCoinToAudit(uint8_t chuteNum) override;
   bool addCoin(uint8_t chuteNum) override;
   void setRolloverValue(uint8_t v) override        { rolloverValue_ = v; }
   void update(unsigned long currentTime) override;
   void stopAllAudio() override;
   void readStoredParameters() override;
   void playCallout(uint8_t track) override;

private:
   MachineSettings settings_;

   AudioHandler audioHandler_;
#if defined(RPU_OS_USE_WAV_TRIGGER)
   WavTriggerHandler wavHandler_;
#endif

   unsigned long nextSoundEffectTime_ = 0;
   uint8_t       rolloverValue_       = 2;
   unsigned long currentTime_         = 0;
   uint8_t       chuteCoinsInProgress_[3] = {};

   static const unsigned short kChuteAuditByte[3];

   static uint8_t readSetting(int addr, uint8_t defaultValue);
   static uint8_t switchToChuteNum(uint8_t switchHit);
};
