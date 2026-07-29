/**************************************************************************
 * MatchMode.cpp
 **************************************************************************/

#include "MatchMode.h"
#include "SoundEffects.h"
#include "Trident2020.h"

void MatchMode::enter(unsigned long currentTime) {
   CurrentTime            = currentTime;
   MatchSequenceStartTime = currentTime;
   MatchDelay             = 1500;
   MatchDigit             = (uint8_t)(currentTime % 10);
   NumMatchSpins          = 0;
   ScoreMatches           = 0;
   machine_->setLampState(LAMP_MATCH, true, 0);
   machine_->setDisableFlippers(true);
   machine_->setLampState(LAMP_BALL_IN_PLAY, false);
}

TopState MatchMode::update(unsigned long currentTime) {
   CurrentTime = currentTime;

   if (!machine_->getMatchFeature()) {
      return TopState::Attract;
   }

   uint8_t numPlayers = machine_->getLastGameNumPlayers();

   if (NumMatchSpins < 40) {
      if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
         MatchDigit += 1;
         if (MatchDigit > 9) {
            MatchDigit = 0;
         }
         machine_->playSoundEffect(SOUND_EFFECT_MATCH_SPIN);
         machine_->setDisplayBallInPlay((int)MatchDigit * 10);
         MatchDelay += 50 + 4 * NumMatchSpins;
         NumMatchSpins += 1;
         machine_->setLampState(LAMP_MATCH, (NumMatchSpins % 2) != 0, 0);
         if (NumMatchSpins == 40) {
            machine_->setLampState(LAMP_MATCH, false);
            MatchDelay = CurrentTime - MatchSequenceStartTime;
         }
      }
   }

   if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
      if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
         uint8_t playerIndex = NumMatchSpins - 40;
         if ((numPlayers > playerIndex) &&
             ((machine_->getLastGameScore(playerIndex) / 10) % 10) == MatchDigit) {
            ScoreMatches |= (1 << playerIndex);
            machine_->addSpecialCredit();
            MatchDelay += 1000;
            NumMatchSpins += 1;
            machine_->setLampState(LAMP_MATCH, true);
         } else {
            NumMatchSpins += 1;
         }
         if (NumMatchSpins == 44) {
            MatchDelay += 5000;
         }
      }
   }

   if (NumMatchSpins > 43) {
      if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
         return TopState::Attract;
      }
   }

   for (int count = 0; count < 4; count++) {
      if ((ScoreMatches >> count) & 0x01) {
         if ((CurrentTime / 200) % 2 != 0) {
            machine_->setDisplayBlank(count, machine_->getDisplayBlank(count) & 0x0F);
         } else {
            machine_->setDisplayBlank(count, machine_->getDisplayBlank(count) | 0x30);
         }
      }
   }

   return TopState::Match;
}
