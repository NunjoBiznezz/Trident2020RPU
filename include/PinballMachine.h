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

   // --- Solenoids / flippers ---
   virtual void disableSolenoidStack()                                 {}
   virtual void setDisableFlippers(bool disable)                       { (void)disable; }

   // --- Misc ---
   virtual void setRolloverValue(uint8_t v)                           { (void)v; }
   virtual void update(unsigned long currentTime)                     { (void)currentTime; }
   virtual void readStoredParameters()                                 {}
};
