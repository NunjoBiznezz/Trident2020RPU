/**************************************************************************
 * PinballMachine.h
 *
 * Abstract interface for machine-level operations: audio, credit
 * management, and hardware I/O (displays, lamps, solenoids).
 * Game-rules classes hold a PinballMachine* and never call RPU_*
 * directly — all hardware access goes through this interface.
 **************************************************************************/

#pragma once
#include <stdint.h>

class PinballMachine {
public:
   virtual ~PinballMachine() = default;

   // --- Audio ---
   virtual void playSoundEffect(uint8_t soundEffectNum)                = 0;
   virtual void playBackgroundSong(unsigned short songNum)             = 0;
   virtual void playBackgroundSongBasedOnBall(uint8_t ballNum)         = 0;
   virtual void stopAllAudio()                                         {}
   virtual void playCallout(uint8_t track)                             { (void)track; }

   // --- Credits / coins ---
   virtual void addCredit(bool playSound = false, uint8_t numToAdd = 1) = 0;
   virtual void addSpecialCredit()                                     = 0;
   virtual void addCoinToAudit(uint8_t chuteNum)                      = 0;
   virtual bool addCoin(uint8_t chuteNum)                             { return false; }

   // --- Displays ---
   virtual void setDisplay(uint8_t display, unsigned long value,
                            bool blankLeadingZeros = false,
                            uint8_t minimumDigits  = 0)               { (void)display; (void)value; (void)blankLeadingZeros; (void)minimumDigits; }
   virtual void setDisplayCredits(uint8_t credits,
                                   bool showCredits = true)           { (void)credits; (void)showCredits; }
   virtual void setDisplayBallInPlay(uint8_t ball,
                                      bool showBall = true)           { (void)ball; (void)showBall; }

   // --- Lamps ---
   virtual void turnOffAllLamps()                                      {}
   virtual void setLampState(uint8_t lamp, bool on,
                              uint8_t dimmer = 0, uint16_t flashRate = 0) { (void)lamp; (void)on; (void)dimmer; (void)flashRate; }

   // --- Solenoids / flippers ---
   virtual void disableSolenoidStack()                                 {}
   virtual void enableSolenoidStack()                                  {}
   virtual void setDisableFlippers(bool disable)                       { (void)disable; }
   virtual void pushToSolenoidStack(uint8_t sol, uint8_t duration)     { (void)sol; (void)duration; }

   // --- Switches ---
   virtual uint8_t pullFirstFromSwitchStack()                          { return 0xFF; }
   virtual bool    readSingleSwitchState(uint8_t sw)                   { (void)sw; return false; }
   virtual bool    getUpDownSwitchState()                              { return false; }

   // --- Displays (additional) ---
   virtual void    setDisplayBlank(uint8_t display, uint8_t mask)      { (void)display; (void)mask; }
   virtual void    cycleAllDisplays(unsigned long t, uint8_t curValue) { (void)t; (void)curValue; }

   // --- Multi-player score display ---
   // Store one player's score (display copy only; game owns the authoritative value).
   virtual void setPlayerScore(uint8_t player, unsigned long score)    { (void)player; (void)score; }
   // Render all player score displays, handling overflow scroll, flash, dash, and
   // score-override animations.  overrideValues may be nullptr when overrideStatus == 0.
   virtual void showScores(uint8_t displayToUpdate, uint8_t numPlayers,
                            bool flashCurrent, bool dashCurrent,
                            unsigned long allScoresShowValue,
                            unsigned long currentTime, unsigned long lastTimeScoreChanged,
                            uint8_t overrideStatus, const unsigned long* overrideValues) {
      (void)displayToUpdate; (void)numPlayers; (void)flashCurrent; (void)dashCurrent;
      (void)allScoresShowValue; (void)currentTime; (void)lastTimeScoreChanged;
      (void)overrideStatus; (void)overrideValues;
   }

   // --- Coin lockout ---
   virtual void    setCoinLockout(bool lock)                           { (void)lock; }

   // --- Sound card (SB100 / S&T / Dash-51) ---
   virtual void    playSoundCardEffect(uint8_t sound)                  { (void)sound; }

   // --- CPC (coins-per-credit) ---
   virtual uint8_t getCPCSelection(uint8_t chuteNum)                  { (void)chuteNum; return 4; }
   virtual uint8_t getCPCPairCoins(uint8_t pairIndex)                 { (void)pairIndex; return 1; }
   virtual uint8_t getCPCPairCredits(uint8_t pairIndex)               { (void)pairIndex; return 1; }
   virtual void    setCPCSelection(uint8_t chuteNum, uint8_t pairIdx) { (void)chuteNum; (void)pairIdx; }

   // --- EEPROM (RPU wrappers) ---
   virtual uint8_t       readByteFromEEProm(uint16_t addr)            { (void)addr; return 0xFF; }
   virtual void          writeByteToEEProm(uint16_t addr, uint8_t val){ (void)addr; (void)val; }
   virtual unsigned long readULFromEEProm(uint16_t addr)              { (void)addr; return 0; }
   virtual void          writeULToEEProm(uint16_t addr, unsigned long val) { (void)addr; (void)val; }

   // --- Dim level ---
   virtual void    setDimDivisor(uint8_t level1, uint8_t level2)       { (void)level1; (void)level2; }

   // --- Self-test debounce ---
   virtual unsigned long getSelfTestChangedTime()                      { return 0; }
   virtual void          setSelfTestChangedTime(unsigned long t)       { (void)t; }

   // --- Misc ---
   virtual void setRolloverValue(uint8_t v)                           { (void)v; }
   virtual void update(unsigned long currentTime)                     { (void)currentTime; }
   virtual void readStoredParameters()                                 {}
};
