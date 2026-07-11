/**************************************************************************
 * AttractMode.cpp
 *
 * Attract mode runs between games. It cycles through three display phases
 * on the head (score displays + ball-in-play):
 *
 *   Phase 1 — First 5 seconds:
 *     Firmware version splash written in enter() using compile-time constants
 *     from BuildVersion.h (game) and RPU.h (OS). Nothing updates during this
 *     window — the displays simply hold the values set at entry.
 *
 *   Phase 2 — Rotating 8-second slots (after the first 5 s):
 *     Slot 0: Trident 2020 all-time high score. Ball-in-play shows "20"
 *             so the operator knows which rule set the score belongs to.
 *     Slot 1: Original Trident all-time high score. Ball-in-play shows "1".
 *     Slot 2: Last-game scores with GAME OVER lamp. Player lamps chase to
 *             indicate how many players participated.
 *
 * lastHeadMode_ tracks which phase is active so display writes happen only
 * on transitions (except the player-lamp chase, which updates every tick).
 *   1 = version splash (first 5 s)
 *   2 = Trident 2020 high score
 *   3 = Original Trident high score
 *   4 = Game Over / last-game scores
 *
 * The playfield runs lamp animations independently of the head display.
 * Each animation plays REPEAT_LAMP_ANIMATIONS times before advancing to
 * the next sequence.
 **************************************************************************/

#include "AttractMode.h"
#include "BuildVersion.h"
#include "LampAnimation.h"
#include "RPU.h"
#include "Trident.h"

#if defined(DEBUG_MESSAGES) && defined(DEBUG_PORT)
#  define DEBUG_MESSAGE(x) DEBUG_PORT.write(x)
#else
#  define DEBUG_MESSAGE(x)
#endif

// How many times each lamp animation plays before advancing to the next.
static constexpr int REPEAT_LAMP_ANIMATIONS = 3;


void AttractMode::enter(unsigned long currentTime) {
   machine_->playSoundCardEffect(0);
   machine_->disableSolenoidStack();
   machine_->turnOffAllLamps();
   machine_->setDisableFlippers(true);
   DEBUG_MESSAGE("Entering Attract Mode\n\r");
   lastPlayfieldMode_ = 0;
   enterTime_         = currentTime;

   // Snapshot last-game results so the display is stable during attract.
   savedNumPlayers_ = machine_->getLastGameNumPlayers();
   for (uint8_t i = 0; i < 4; i++) savedScores_[i] = machine_->getLastGameScore(i);

   // Show firmware version immediately so it is visible before update() runs.
   // Displays 0/1 = game major/minor; displays 2/3 = RPU OS major/minor.
   machine_->setDisplay(0, TRIDENT2020_MAJOR_VERSION);
   machine_->setDisplay(1, TRIDENT2020_MINOR_VERSION);
   machine_->setDisplay(2, RPU_OS_MAJOR_VERSION);
   machine_->setDisplay(3, RPU_OS_MINOR_VERSION);
   machine_->setDisplayCredits(machine_->getCredits(), true);
   machine_->setDisplayBallInPlay(0, true);
   lastHeadMode_ = 1;
}

TopState AttractMode::update(unsigned long currentTime) {

   // -----------------------------------------------------------------------
   // Head display — score displays + ball-in-play
   // -----------------------------------------------------------------------

   if ((currentTime - enterTime_) < 5000) {
      // Hold the version splash written in enter() — nothing to do each tick.
   } else {
      // After the splash: rotate through three 8-second slots.
      // Ball-in-play shows the rule-set number on high-score slots so the
      // operator can tell which score belongs to which set of rules.
      uint8_t headSlot = (uint8_t)((currentTime / 8000) % 3);
      if (headSlot == 0) {
         // Slot 0: Trident 2020 high score — ball-in-play = "20"
         if (lastHeadMode_ != 2) {
            machine_->setLampState(LAMP_HIGH_SCORE_TO_DATE, true, 0, 250);
            machine_->setLampState(LAMP_GAME_OVER, false);
            for (uint8_t i = 0; i < 4; i++) machine_->setLampState(LAMP_PLAYER_1 + i, false);
            for (uint8_t i = 0; i < 4; i++) machine_->setDisplay(i, machine_->getHighScore(), true, 2);
            machine_->setDisplayBallInPlay(20, true);
         }
         lastHeadMode_ = 2;
      } else if (headSlot == 1) {
         // Slot 1: Original Trident high score — ball-in-play = "1"
         if (lastHeadMode_ != 3) {
            machine_->setLampState(LAMP_HIGH_SCORE_TO_DATE, true, 0, 250);
            machine_->setLampState(LAMP_GAME_OVER, false);
            for (uint8_t i = 0; i < 4; i++) machine_->setLampState(LAMP_PLAYER_1 + i, false);
            for (uint8_t i = 0; i < 4; i++) machine_->setDisplay(i, machine_->getOriginalHighScore(), true, 2);
            machine_->setDisplayBallInPlay(1, true);
         }
         lastHeadMode_ = 3;
      } else {
         // Slot 2: last-game scores with cycling player lamps
         if (lastHeadMode_ != 4) {
            machine_->setLampState(LAMP_HIGH_SCORE_TO_DATE, false);
            machine_->setLampState(LAMP_GAME_OVER, true);
            machine_->setDisplayCredits(machine_->getCredits(), true);
            machine_->setDisplayBallInPlay(0, true);
            for (uint8_t i = 0; i < 4; i++) {
               if (savedNumPlayers_ > 0 && i >= savedNumPlayers_) machine_->setDisplayBlank(i, 0x00);
               else                                                machine_->setDisplay(i, savedScores_[i], true, 2);
            }
         }
         // Player lamps chase every 250 ms regardless of transition state.
         uint8_t activePlayer = (uint8_t)((currentTime / 250) % 4);
         for (uint8_t i = 0; i < 4; i++) machine_->setLampState(LAMP_PLAYER_1 + i, i == activePlayer);
         lastHeadMode_ = 4;
      }
   }

   // -----------------------------------------------------------------------
   // Playfield lamp animations
   // -----------------------------------------------------------------------

   if (lastPlayfieldMode_ != 1) {
      lastPlayfieldMode_ = 1;
      animCount_         = 0;
      animNum_           = 0;
      animStep_          = 0;
      lastAnimFrameTime_ = 0;
   }
   if ((currentTime - lastAnimFrameTime_) >= 100) {
      lastAnimFrameTime_ = currentTime;
      const uint8_t* animBytes = PeekAnimationBytes(animNum_, animStep_);
      machine_->setLampAnimationBytes(animBytes, NUM_LAMP_ANIMATION_BYTES);
      if (++animStep_ >= LAMP_ANIMATION_STEPS) {
         animStep_ = 0;
         animCount_++;
         if (animCount_ > REPEAT_LAMP_ANIMATIONS) {
            animCount_ = 0;
            if (++animNum_ >= NUM_LAMP_ANIMATIONS) {
               animNum_ = 0;
            }
         }
      }
   }

   // -----------------------------------------------------------------------
   // Switch handling
   // -----------------------------------------------------------------------

   uint8_t switchHit;
   while ((switchHit = machine_->pullFirstFromSwitchStack()) != PinballMachine::SWITCH_STACK_EMPTY) {
      if (switchHit == SW_CREDIT_RESET) {
         if (machine_->getCredits() >= 1 || machine_->getFreePlayMode()) {
            return TopState::Game;
         }
      }
      if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
         machine_->addCoinToAudit(switchHit);
         machine_->addCredit(true, 1);
      }
      if (switchHit == SW_SELF_TEST_SWITCH &&
          (currentTime - selfTestLastPressedTime_) > 250) {
         selfTestLastPressedTime_ = currentTime;
         return TopState::HardwareTest;
      }
   }

   return TopState::Attract;
}
