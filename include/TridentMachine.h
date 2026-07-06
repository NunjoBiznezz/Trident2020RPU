/**************************************************************************
 * TridentMachine.h
 *
 * Concrete PinballMachine implementation for the Trident hardware.
 * Owns the audio subsystem (AudioHandler, WavTriggerHandler) and
 * implements all credit and coin-mech operations. Constructed once in
 * main.cpp; other classes hold a PinballMachine* or TridentMachine*
 * depending on whether they need the concrete pointer-getter API.
 **************************************************************************/

#pragma once
#include "PinballMachine.h"
#include "AudioHandler.h"
#if defined(RPU_OS_USE_WAV_TRIGGER)
#include "WavTriggerHandler.h"
#endif
#include <stdint.h>

struct GameContext;   // full definition in Trident2020Game.h

class TridentMachine : public PinballMachine {
public:
   TridentMachine() = default;

   void setContext(GameContext& ctx)   { ctx_ = &ctx; }

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

   // Non-virtual pointer getters for SelfTestMode adjustment variables.
   uint8_t* soundSelectorPtr()  { return &soundSelector_; }
   uint8_t* musicVolumePtr()    { return &musicVolume_; }
   uint8_t* sfxVolumePtr()      { return &sfxVolume_; }
   uint8_t* calloutsVolumePtr() { return &calloutsVolume_; }

private:
   GameContext* ctx_ = nullptr;

   AudioHandler audioHandler_;
#if defined(RPU_OS_USE_WAV_TRIGGER)
   WavTriggerHandler wavHandler_;
#endif

   uint8_t       soundSelector_  = 3;   // SOUND_SELECTOR_TRIDENT2020
   uint8_t       musicVolume_    = 10;
   uint8_t       sfxVolume_      = 10;
   uint8_t       calloutsVolume_ = 10;
   unsigned long nextSoundEffectTime_ = 0;
   uint8_t       rolloverValue_  = 2;
   unsigned long currentTime_    = 0;
   uint8_t       chuteCoinsInProgress_[3] = {};

   static const unsigned short kChuteAuditByte[3];

   static uint8_t readSetting(int addr, uint8_t defaultValue);
   static uint8_t switchToChuteNum(uint8_t switchHit);
};
