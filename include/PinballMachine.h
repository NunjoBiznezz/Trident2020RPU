/**************************************************************************
 * PinballMachine.h
 *
 * Abstract interface for machine-level operations (audio and credit
 * management) that game rules classes need but that are not specific to
 * one game variant. Inject a concrete implementation via setMachine()
 * on the game class so that the game code never links directly against
 * main.cpp free functions.
 **************************************************************************/

#pragma once
#include <stdint.h>

class PinballMachine {
public:
   virtual void playSoundEffect(uint8_t soundEffectNum)         = 0;
   virtual void playBackgroundSong(unsigned short songNum)      = 0;
   virtual void playBackgroundSongBasedOnBall(uint8_t ballNum)  = 0;
   virtual void addCredit(bool playSound = false, uint8_t numToAdd = 1) = 0;
   virtual void addSpecialCredit()                              = 0;
   virtual void addCoinToAudit(uint8_t chuteNum)               = 0;
};
