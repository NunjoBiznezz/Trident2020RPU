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
   void stopAllAudio() override;
   void playCallout(uint8_t track) override;

   void addCredit(bool playSound, uint8_t numToAdd) override;
   void addSpecialCredit() override;
   void addCoinToAudit(uint8_t chuteNum) override;
   bool addCoin(uint8_t chuteNum) override;

   void setDisplay(uint8_t display, unsigned long value,
                   bool blankLeadingZeros, uint8_t minimumDigits) override;
   void setDisplayCredits(uint8_t credits, bool showCredits) override;
   void setDisplayBallInPlay(uint8_t ball, bool showBall) override;

   void turnOffAllLamps() override;
   void setLampState(uint8_t lamp, bool on, uint8_t dimmer, uint16_t flashRate) override;
   void disableSolenoidStack() override;
   void enableSolenoidStack() override;
   void setDisableFlippers(bool disable) override;
   void pushToSolenoidStack(uint8_t sol, uint8_t duration) override;

   uint8_t pullFirstFromSwitchStack() override;
   bool    readSingleSwitchState(uint8_t sw) override;
   bool    getUpDownSwitchState() override;

   void    setDisplayBlank(uint8_t display, uint8_t mask) override;
   void    cycleAllDisplays(unsigned long t, uint8_t curValue) override;

   void    setCoinLockout(bool lock) override;
   void    playSoundCardEffect(uint8_t sound) override;

   uint8_t getCPCSelection(uint8_t chuteNum) override;
   uint8_t getCPCPairCoins(uint8_t pairIndex) override;
   uint8_t getCPCPairCredits(uint8_t pairIndex) override;
   void    setCPCSelection(uint8_t chuteNum, uint8_t pairIdx) override;

   uint8_t       readByteFromEEProm(uint16_t addr) override;
   void          writeByteToEEProm(uint16_t addr, uint8_t val) override;
   unsigned long readULFromEEProm(uint16_t addr) override;
   void          writeULToEEProm(uint16_t addr, unsigned long val) override;

   void    setDimDivisor(uint8_t level1, uint8_t level2) override;

   void setRolloverValue(uint8_t v) override        { rolloverValue_ = v; }
   void update(unsigned long currentTime) override;
   void readStoredParameters() override;

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

   bool    cpcSelectionsRead_  = false;
   uint8_t cpcSelection_[3]    = {};

   static const unsigned short kChuteAuditByte[3];
   static const uint8_t        kCPCPairs[9][2];
   static const uint16_t       kCPCChuteByte[3];

   void ensureCPCRead();

   static uint8_t readSetting(int addr, uint8_t defaultValue);
   static uint8_t switchToChuteNum(uint8_t switchHit);
};
