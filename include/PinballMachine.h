/**************************************************************************
 * PinballMachine.h
 *
 * Concrete machine class for the Trident hardware. Owns the audio
 * subsystem (AudioHandler + WavTriggerHandler) and the single
 * MachineSettings instance. Wraps every RPU_* call so that game-rules
 * objects (Trident2020Game, TridentGame, AttractMode, …) never touch
 * hardware registers directly.
 *
 * All methods are non-virtual — this is the only machine implementation.
 * The interface can be abstracted again later if a second target is added.
 **************************************************************************/

#pragma once
#include "AudioHandler.h"
#include "MachineSettings.h"
#if defined(RPU_OS_USE_WAV_TRIGGER)
#include "WavTriggerHandler.h"
#endif
#include <stdint.h>

class PinballMachine {
public:
   // Returned by pullFirstFromSwitchStack() when the queue is empty.
   static constexpr uint8_t SWITCH_STACK_EMPTY = 0xFF;

   // Digit-enable mask for all 6 score-display positions (1 = show, 0 = blank).
   // Use as the default mask for setDisplayBlank to enable every digit position.
   static constexpr uint8_t ALL_DIGITS_MASK = 0x3F;

   PinballMachine() = default;

   // One-time hardware init; call from setup() before the main loop.
   void init(unsigned long currentTime);

   // Queue a WAV notification during boot diagnostics (before game loop).
   void queueDiagNotification(unsigned short notificationNum, unsigned long currentTime);

   // -----------------------------------------------------------------------
   // Audio
   //
   // Two audio paths may be active simultaneously:
   //   WAV Trigger  — high-quality samples on SD card (RPU_OS_USE_WAV_TRIGGER)
   //   Sound card   — original Stern SB-100 / Squawk & Talk / Dash-51 hardware
   //
   // playSoundEffect / playBackgroundSong / playCallout target the WAV Trigger
   // path (managed by WavTriggerHandler). playSoundCardEffect targets the
   // native sound card path (managed by AudioHandler).
   // -----------------------------------------------------------------------

   // One-shot sound effect. soundEffectNum maps directly to the WAV file
   // track number on the SD card, or to an audioHandler command when the
   // original sound card is selected.
   void playSoundEffect(uint8_t soundEffectNum);

   // Start a looping background song on the WAV Trigger music channel.
   void playBackgroundSong(unsigned short songNum);

   // Stop all active audio on every WAV Trigger channel plus the sound card.
   void stopAllAudio();

   // Play a voice callout on the WAV Trigger notification channel. Callouts
   // duck the FX channel; the notification queue handles priority ordering.
   void playCallout(uint8_t track);

   // Send a command directly to the native sound card (SB-100, S&T, Dash-51).
   // sound = 0 typically silences. Separate from the WAV Trigger path.
   void playSoundCardEffect(uint8_t sound);

   // -----------------------------------------------------------------------
   // Credits and coins
   // -----------------------------------------------------------------------

   // Add numToAdd credits, cap at maximumCredits, write to EEPROM.
   // Plays a coin-drop sound if playSound is true.
   void addCredit(bool playSound = false, uint8_t numToAdd = 1);

   // Award a match/special credit: addCredit + fire knocker + update replay audit.
   void addSpecialCredit();

   // Increment the per-chute coin audit counter in EEPROM.
   void addCoinToAudit(uint8_t chuteNum);

   // Full coin insertion: audit the chute, play a random coin sound, add one
   // credit. Returns false if chuteNum is out of range.
   bool addCoin(uint8_t chuteNum);

   // Engage or release the coin-door lockout coil.
   void setCoinLockout(bool lock);

   // -----------------------------------------------------------------------
   // Score displays
   //
   // Four 6-digit score displays (index 0–3) plus a 2-digit credits display
   // and a 2-digit ball-in-play display. The RPU display ISR strobes at ~320 Hz.
   //
   // setDisplay handles scores above RPU_OS_MAX_DISPLAY_SCORE by scrolling
   // the value across the display.
   // -----------------------------------------------------------------------

   // Write a static value. blankLeadingZeros suppresses leading zeros;
   // minimumDigits forces at least that many digits from the right.
   void setDisplay(uint8_t display, unsigned long value,
                   bool blankLeadingZeros = false, uint8_t minimumDigits = 0);

   // Write a value and make the display flash at period ms.
   void setDisplayFlash(uint8_t display, unsigned long value, uint16_t period = 500);

   // Write to the 2-digit credits display; pass showCredits=false to blank it.
   void setDisplayCredits(uint8_t credits, bool showCredits = true);

   // Write to the 2-digit ball-in-play display; pass showBall=false to blank it.
   // During attract mode this is repurposed for rule-set identification.
   void setDisplayBallInPlay(uint8_t ball, bool showBall = true);

   // Set the digit-enable mask for a score display. Each bit controls one digit
   // position (1=show, 0=blank). Default disables all positions.
   void    setDisplayBlank(uint8_t display, uint8_t mask = 0x00);
   uint8_t getDisplayBlank(uint8_t display);

   // Cycle all four score displays through a diagnostic sweep (hardware test).
   void cycleAllDisplays(unsigned long t, uint8_t curValue);

   // -----------------------------------------------------------------------
   // Lamps
   // -----------------------------------------------------------------------

   void turnOffAllLamps();

   // Set a single lamp.
   //   dimmer    — 0 = full brightness, 1 = dim (controlled by dimLevel setting)
   //   flashRate — period in ms; 0 = steady
   void setLampState(uint8_t lamp, bool on, uint8_t dimmer = 0, uint16_t flashRate = 0);

   // Push a raw lamp-state buffer from PeekAnimationBytes() to the RPU lamp
   // animation register. Used by AttractMode for playfield sequences.
   void setLampAnimationBytes(const uint8_t* bytes, uint8_t count);

   // Set the lamp dim divisor for dimmer slot level1. Typical values: 2 or 3.
   void setDimDivisor(uint8_t level1, uint8_t level2);

   // -----------------------------------------------------------------------
   // Solenoids and flippers
   //
   // duration values are in units of 1/120 s (~8.3 ms each).
   // disableOverride = true fires even when the solenoid stack is disabled
   // (used for knocker, outhole during ball save, etc.).
   // -----------------------------------------------------------------------

   void disableSolenoidStack();
   void enableSolenoidStack();
   void setDisableFlippers(bool disable);

   void pushToSolenoidStack(uint8_t sol, uint8_t duration,
                            bool disableOverride = false);
   void pushToTimedSolenoidStack(uint8_t sol, uint8_t numPushes,
                                  unsigned long whenToFire,
                                  bool disableOverride = false);

   // -----------------------------------------------------------------------
   // Switches
   //
   // The RPU switch stack is a debounced FIFO filled by the switch ISR.
   // Drain it every loop tick with a while loop until SWITCH_STACK_EMPTY.
   // -----------------------------------------------------------------------

   // Remove and return the oldest pending switch hit, or SWITCH_STACK_EMPTY.
   uint8_t pullFirstFromSwitchStack();

   // Read the instantaneous debounced state of a single switch without
   // consuming it from the stack. Useful for outhole, saucer checks, etc.
   bool readSingleSwitchState(uint8_t sw);

   // -----------------------------------------------------------------------
   // Audit and persistence — semantic operations; no raw EEPROM addresses
   // leak into game code. All RPU EEPROM addresses stay inside PinballMachine.
   // -----------------------------------------------------------------------

   // Determine if a player may be added to a game.
   bool canAddPlayer(uint8_t coinsRequired = 1) const {
      return settings_.credits >= coinsRequired || settings_.freePlayMode;
   }

   // Decrement settings_.credits by one, save to EEPROM.
   // Symmetric to addCredit(). Use getCredits() for the updated value.
   void decrementCredits();

   // Persist settings_.credits to EEPROM (use when the credit count was
   // loaded from EEPROM and only a flush is needed, not an adjustment).
   void saveCredits();

   // Increment the total-games-played audit counter.
   void recordGamePlayed();

   // Add count to the replay audit counter. Used when a player earns
   // multiple replays at once (e.g. 3 for beating the high score).
   void addReplayAudit(uint8_t count);

   // Record a new Trident 2020 high score: updates the in-memory field,
   // persists to EEPROM, and increments the beaten counter.
   void saveHighScore(unsigned long score);

   // Clear the resetScoresToClearVersion flag once the game has acted on it.
   void acknowledgeResetScores();

   // -----------------------------------------------------------------------
   // Miscellaneous
   // -----------------------------------------------------------------------

   // Set the rollover lane value used by the original-sound-card path to
   // determine how many tones to play on an inlane hit.
   void setRolloverValue(uint8_t v) { rolloverValue_ = v; }

   // RPU loop housekeeping — call at start and end of each loop() tick in place
   // of RPU_DataRead / RPU_Update so callers need no RPU headers.
   void readInputs();
   void flushOutputs();

   // Called every loop() tick to service audio handlers and timed solenoids.
   // Must be called before any game-mode update().
   void update(unsigned long currentTime);

   // Re-read all operator settings from EEPROM into MachineSettings. Called
   // on entry to attract mode after the operator exits the adjustment menus.
   void readStoredParameters();

   // -----------------------------------------------------------------------
   // Accessors
   // -----------------------------------------------------------------------

   uint8_t          getCredits() const       { return settings_.credits; }
   bool             getFreePlayMode() const  { return settings_.freePlayMode; }
   bool             getMatchFeature() const  { return settings_.matchFeature; }
   MachineSettings& getSettings();

   // Hardware capability queries — use these instead of RPU_* constants.
   int           getMaxLamps()              const;
   uint8_t       getHardwareMajorVersion()  const;
   uint8_t       getHardwareMinorVersion()  const;
   unsigned long getMaxDisplayScore()       const;
   uint8_t       getNumDisplayDigits()      const;
   uint8_t       getTotalDisplayDigits()    const;

   unsigned long getHighScore() const         { return settings_.trident2020Settings.highScore; }
   unsigned long getOriginalHighScore() const { return settings_.tridentSettings.highScore; }

   // -----------------------------------------------------------------------
   // Last-game result — stored between games for attract mode display.
   //
   // Trident2020Game calls setLastGameResult() once all scores have been
   // shown. AttractMode reads the values in enter() so they remain stable
   // for the entire attract cycle.
   // -----------------------------------------------------------------------

   // Store the final scores for up to 4 players. scores must point to an
   // array of at least numPlayers elements.
   void setLastGameResult(uint8_t numPlayers, const unsigned long* scores);

   // Returns the number of players in the last completed game (0 if none).
   uint8_t getLastGameNumPlayers() const { return lastGameNumPlayers_; }

   // Returns the final score for the given player index (0-based).
   unsigned long getLastGameScore(uint8_t player) const {
      return (player < 4) ? lastGameScores_[player] : 0;
   }

private:
   MachineSettings settings_;

   AudioHandler audioHandler_;
#if defined(RPU_OS_USE_WAV_TRIGGER)
   WavTriggerHandler wavHandler_;
#endif

   unsigned long nextSoundEffectTime_    = 0;
   uint8_t       rolloverValue_          = 2;
   unsigned long currentTime_            = 0;

   // Per-display scroll state for values above RPU_OS_MAX_DISPLAY_SCORE.
   unsigned long displayValue_[4]        = {};
   unsigned long displayValueSetTime_[4] = {};
   uint8_t       displayScrollPhase_[4]  = {0xFF, 0xFF, 0xFF, 0xFF};

   bool haveSettings_ = false;

   // Last completed game results (for attract mode).
   unsigned long lastGameScores_[4]  = {};
   uint8_t       lastGameNumPlayers_ = 0;

   // Read a byte from game EEPROM; write defaultValue if unset (0xFF).
   static uint8_t  readSetting(int addr, uint8_t defaultValue);
   // Read a uint32_t from game EEPROM; write defaultValue if unset (0xFFFFFFFF).
   static uint32_t readULSetting(int addr, uint32_t defaultValue = 0);

   // Map a coin switch number to a 0-based chute index.
   static uint8_t switchToChuteNum(uint8_t switchHit);

   // Helpers for the large-score scrolling display logic.
   static uint8_t magnitudeOfScore(unsigned long score);
   static uint8_t getDisplayMask(uint8_t numDigits);
};
