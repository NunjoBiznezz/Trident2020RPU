/**************************************************************************
 * TridentMachineOps.cpp
 *
 * Concrete PinballMachine for the Trident hardware. Owns the audio
 * subsystem and implements all credit / coin-mech operations that were
 * previously free functions in main.cpp.
 **************************************************************************/

#include "TridentMachineOps.h"
#include "Trident2020Game.h"
#include "RPU.h"
#include "SelfTestAndAudit.h"
#include "SoundEffects.h"
#include "Trident2020.h"
#include <EEPROM.h>

// ---------------------------------------------------------------------------
// Static data
// ---------------------------------------------------------------------------

const unsigned short TridentMachineOps::kChuteAuditByte[3] = {
   RPU_CHUTE_1_COINS_START_BYTE,
   RPU_CHUTE_2_COINS_START_BYTE,
   RPU_CHUTE_3_COINS_START_BYTE,
};

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

uint8_t TridentMachineOps::readSetting(int addr, uint8_t defaultValue) {
   uint8_t value = EEPROM.read(addr);
   if (value == 0xFF) {
      EEPROM.write(addr, defaultValue);
      return defaultValue;
   }
   return value;
}

uint8_t TridentMachineOps::switchToChuteNum(uint8_t switchHit) {
   if (switchHit == SW_COIN_2) return 1;
   if (switchHit == SW_COIN_3) return 2;
   return 0;
}

// ---------------------------------------------------------------------------
// Init / lifecycle
// ---------------------------------------------------------------------------

void TridentMachineOps::init(unsigned long currentTime) {
   currentTime_ = currentTime;
   audioHandler_.initDevices();
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.init();
#endif
   stopAllAudio();
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.setMusicDuckingGain(16);
   wavHandler_.queueSound(SOUND_EFFECT_TRIDENT_INTRO, currentTime + 5000);
#endif
}

void TridentMachineOps::queueDiagNotification(unsigned short notificationNum,
                                               unsigned long  currentTime) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.queuePrioritizedNotification(notificationNum, 0, 10, currentTime);
#else
   (void)notificationNum;
   (void)currentTime;
#endif
}

// ---------------------------------------------------------------------------
// PinballMachine interface
// ---------------------------------------------------------------------------

void TridentMachineOps::update(unsigned long currentTime) {
   currentTime_ = currentTime;
   audioHandler_.update(currentTime);
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.update(currentTime);
#endif
}

void TridentMachineOps::stopAllAudio() {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.stopAllAudio();
#endif
   audioHandler_.stopAllSoundFX();
}

void TridentMachineOps::readStoredParameters() {
   soundSelector_ = readSetting(EEPROM_SOUND_SELECTOR_BYTE, 3);
   switch (soundSelector_) {
   case SOUND_SELECTOR_NONE:
   case SOUND_SELECTOR_ORIGINAL:
   case SOUND_SELECTOR_TRIDENT2020:
      break;
   default:
      soundSelector_ = SOUND_SELECTOR_TRIDENT2020;
   }
   if (soundSelector_ > 3) soundSelector_ = 3;

   musicVolume_ = readSetting(EEPROM_MUSIC_VOLUME_BYTE, 10);
   if (musicVolume_ > 10) musicVolume_ = 10;

   sfxVolume_ = readSetting(EEPROM_SFX_VOLUME_BYTE, 10);
   if (sfxVolume_ > 10) sfxVolume_ = 10;

   calloutsVolume_ = readSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10);
   if (calloutsVolume_ > 10) calloutsVolume_ = 10;

#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.setMusicVolume(musicVolume_);
   wavHandler_.setSoundFXVolume(sfxVolume_);
   wavHandler_.setNotificationsVolume(calloutsVolume_);
#endif
}

void TridentMachineOps::playCallout(uint8_t track) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
   wavHandler_.playSound(track, 10);
#else
   (void)track;
#endif
}

void TridentMachineOps::playBackgroundSong(unsigned short songNum) {
   if (musicVolume_ != 0 && soundSelector_ == SOUND_SELECTOR_TRIDENT2020) {
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wavHandler_.playBackgroundSong(songNum, true);
#else
      (void)songNum;
#endif
   }
}

void TridentMachineOps::playBackgroundSongBasedOnBall(uint8_t ballNum) {
   uint8_t ballsPerGame = ctx_ ? *ctx_->ballsPerGame : 3;
   if (ballNum == 1) {
      playBackgroundSong(SOUND_EFFECT_BACKGROUND_1);
   } else if (ballNum == ballsPerGame) {
      playBackgroundSong(SOUND_EFFECT_BACKGROUND_6);
   } else {
      playBackgroundSong(SOUND_EFFECT_BACKGROUND_2 + currentTime_ % 4);
   }
}

void TridentMachineOps::playSoundEffect(uint8_t soundEffectNum) {
   switch (soundSelector_) {
   case SOUND_SELECTOR_NONE:
      return;

   case SOUND_SELECTOR_ORIGINAL:
      switch (soundEffectNum) {
      case SOUND_EFFECT_ROLLOVER:
      case SOUND_EFFECT_DT_SKILL_SHOT:
      case SOUND_EFFECT_ROLLOVER_SKILL_SHOT:
      case SOUND_EFFECT_SU_SKILL_SHOT:
      case SOUND_EFFECT_LEFT_SPINNER:
      case SOUND_EFFECT_RIGHT_SPINNER:
      case SOUND_EFFECT_DROP_TARGET:
      case SOUND_EFFECT_BALL_OVER:
         audioHandler_.queueSound(0x02, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_LEFT_INLANE:
         for (int count = 0; count < rolloverValue_; count++) {
            audioHandler_.queueSound(0x04, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_RIGHT_INLANE:
         for (int count = 0; count < 6; count++) {
            audioHandler_.queueSound((count < 3) ? 0x04 : 0x10, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_5K:
         for (int count = 0; count < 5; count++) {
            audioHandler_.queueSound(0x04, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_30K:
         for (int count = 0; count < 3; count++) {
            audioHandler_.queueSound(0x08, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_20K:
         for (int count = 0; count < 2; count++) {
            audioHandler_.queueSound(0x08, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_SAUCER_HIT_10K:
         for (int count = 0; count < 1; count++) {
            audioHandler_.queueSound(0x08, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_RIGHT_OUTLANE:
         for (int count = 0; count < 5; count++) {
            audioHandler_.queueSound(0x04, currentTime_ + 200 * count);
            audioHandler_.queueSound(0x00, currentTime_ + 75 + (200 * count));
         }
         break;
      case SOUND_EFFECT_TOP_BUMPER_HIT:
      case SOUND_EFFECT_BOTTOM_BUMPER_HIT:
         audioHandler_.queueSound(0x20, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_SHOOT_AGAIN:
      case SOUND_EFFECT_PLAYER_1_UP:
      case SOUND_EFFECT_PLAYER_2_UP:
      case SOUND_EFFECT_PLAYER_3_UP:
      case SOUND_EFFECT_PLAYER_4_UP:
         audioHandler_.queueSound(0x08, currentTime_);
         audioHandler_.queueSound(0x04, currentTime_ + 75);
         audioHandler_.queueSound(0x00, currentTime_ + 175);
         break;
      case SOUND_EFFECT_BONUS_COUNT:
      case SOUND_EFFECT_2X_BONUS_COUNT:
      case SOUND_EFFECT_3X_BONUS_COUNT:
      case SOUND_EFFECT_4X_BONUS_COUNT:
      case SOUND_EFFECT_5X_BONUS_COUNT:
         audioHandler_.queueSound(0x04, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_UPPER_SLING:
      case SOUND_EFFECT_EXTRA_BALL:
      case SOUND_EFFECT_TILT_WARNING:
         audioHandler_.queueSound(0x10, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_10PT_SWITCH:
      case SOUND_EFFECT_MATCH_SPIN:
      case SOUND_EFFECT_LOWER_SLING:
         audioHandler_.queueSound(0x01, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_DROP_TARGET_CLEAR_1:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_2:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_3:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_4:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_5:
         audioHandler_.queueSound(0x08, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_FIRST_SU_SWITCH_HIT:
      case SOUND_EFFECT_SECOND_SU_SWITCH_HIT:
      case SOUND_EFFECT_THIRD_SU_SWITCH_HIT:
      case SOUND_EFFECT_FOURTH_SU_SWITCH_HIT:
      case SOUND_EFFECT_FIFTH_SU_SWITCH_HIT:
         audioHandler_.queueSound(0x04, currentTime_);
         audioHandler_.queueSound(0x00, currentTime_ + 75);
         break;
      case SOUND_EFFECT_ADD_CREDIT:
      case SOUND_EFFECT_GAME_OVER:
         audioHandler_.queueSound(0x08, currentTime_);
         audioHandler_.queueSound(0x04, currentTime_ + 75);
         audioHandler_.queueSound(0x02, currentTime_ + 150);
         audioHandler_.queueSound(0x01, currentTime_ + 225);
         audioHandler_.queueSound(0x08, currentTime_ + 325);
         audioHandler_.queueSound(0x04, currentTime_ + 400);
         audioHandler_.queueSound(0x02, currentTime_ + 475);
         audioHandler_.queueSound(0x01, currentTime_ + 550);
         audioHandler_.queueSound(0x00, currentTime_ + 650);
         break;
      case SOUND_EFFECT_ADD_PLAYER_1:
      case SOUND_EFFECT_ADD_PLAYER_2:
      case SOUND_EFFECT_ADD_PLAYER_3:
      case SOUND_EFFECT_ADD_PLAYER_4:
      case SOUND_EFFECT_RESCUE_FROM_THE_DEEP:
      case SOUND_EFFECT_TRIDENT_INTRO:
         audioHandler_.queueSound(0x01, currentTime_);
         audioHandler_.queueSound(0x02, currentTime_ + 75);
         audioHandler_.queueSound(0x04, currentTime_ + 150);
         audioHandler_.queueSound(0x08, currentTime_ + 225);
         audioHandler_.queueSound(0x01, currentTime_ + 325);
         audioHandler_.queueSound(0x02, currentTime_ + 400);
         audioHandler_.queueSound(0x04, currentTime_ + 475);
         audioHandler_.queueSound(0x08, currentTime_ + 550);
         audioHandler_.queueSound(0x00, currentTime_ + 650);
         break;
      }
      break;

   case SOUND_SELECTOR_TRIDENT2020:
   default:
#if defined(RPU_OS_USE_WAV_TRIGGER)
      wavHandler_.playSound(soundEffectNum);
#endif
      break;
   }
}

// ---------------------------------------------------------------------------
// Credit and coin operations
// ---------------------------------------------------------------------------

void TridentMachineOps::addCredit(bool playSound, uint8_t numToAdd) {
   if (!ctx_) return;
   uint8_t& credits    = *ctx_->credits;
   uint8_t  maxCredits = *ctx_->maximumCredits;
   if (credits < maxCredits) {
      credits += numToAdd;
      if (credits > maxCredits) credits = maxCredits;
      RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, credits);
      if (playSound) playSoundEffect(SOUND_EFFECT_ADD_CREDIT);
      RPU_SetDisplayCredits(credits, !*ctx_->freePlayMode);
      RPU_SetCoinLockout(false);
   } else {
      RPU_SetDisplayCredits(credits, !*ctx_->freePlayMode);
      RPU_SetCoinLockout(true);
   }
}

void TridentMachineOps::addSpecialCredit() {
   addCredit(false, 1);
   RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, currentTime_, true);
   RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE,
                        RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

void TridentMachineOps::addCoinToAudit(uint8_t chuteNum) {
   if (chuteNum > 2) return;
   unsigned short coinAuditStartByte = kChuteAuditByte[chuteNum];
   RPU_WriteULToEEProm(coinAuditStartByte, RPU_ReadULFromEEProm(coinAuditStartByte) + 1);
}

bool TridentMachineOps::addCoin(uint8_t chuteNum) {
   if (chuteNum > 2) return false;
   uint8_t cpcSelection = GetCPCSelection(chuteNum);

   uint8_t chuteNumToUse;
   for (chuteNumToUse = 0; chuteNumToUse <= chuteNum; chuteNumToUse++) {
      if (GetCPCSelection(chuteNumToUse) == cpcSelection) break;
   }

   playSoundEffect(SOUND_EFFECT_COIN_DROP_1 + (currentTime_ % 3));

   uint8_t cpcCoins          = GetCPCCoins(cpcSelection);
   uint8_t cpcCredits        = GetCPCCredits(cpcSelection);
   uint8_t coinProgressBefore = chuteCoinsInProgress_[chuteNumToUse];
   chuteCoinsInProgress_[chuteNumToUse] += 1;

   bool creditAdded = false;
   if (chuteCoinsInProgress_[chuteNumToUse] == cpcCoins) {
      addCredit(cpcCredits > cpcCoins ? (cpcCredits - coinProgressBefore) : cpcCredits, 1);
      chuteCoinsInProgress_[chuteNumToUse] = 0;
      creditAdded = true;
   } else if (cpcCredits > cpcCoins) {
      addCredit(true, 1);
      creditAdded = true;
   }
   return creditAdded;
}
