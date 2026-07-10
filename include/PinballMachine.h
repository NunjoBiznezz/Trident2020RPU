/**************************************************************************
 * PinballMachine.h
 *
 * Abstract interface for all machine-level hardware operations. Game-rules
 * classes (Trident2020Game, TridentGame, AttractMode, etc.) hold a
 * PinballMachine* and call only these methods — they never call RPU_*
 * functions directly. This keeps game logic testable and portable.
 *
 * TridentMachine is the concrete implementation that delegates to RPU_*
 * and the WAV Trigger handler.
 *
 * Default implementations are no-ops so that test harnesses or stub
 * subclasses only need to override the methods they care about.
 * Pure-virtual methods (marked = 0) must be implemented by every concrete
 * subclass — they represent operations that have no sensible default.
 **************************************************************************/

#pragma once
#include <stdint.h>

class MachineSettings;

class PinballMachine {
public:
   // Sentinel returned by pullFirstFromSwitchStack() when the queue is empty.
   static constexpr uint8_t SWITCH_STACK_EMPTY = 0xFF;

   virtual ~PinballMachine() = default;

   // -----------------------------------------------------------------------
   // Audio
   //
   // Two audio paths exist and may be active simultaneously:
   //   WAV Trigger  — high-quality samples on SD card (RPU_OS_USE_WAV_TRIGGER)
   //   Sound card   — original Stern SB-100 / Squawk & Talk / Dash-51 hardware
   //
   // playSoundEffect / playBackgroundSong / playCallout go to the WAV Trigger
   // path (managed by WavTriggerHandler). playSoundCardEffect goes to the
   // native sound card path (managed by AudioHandler).
   // -----------------------------------------------------------------------

   // One-shot sound effect on the WAV Trigger FX channel. soundEffectNum maps
   // directly to the WAV file track number on the SD card.
   virtual void playSoundEffect(uint8_t soundEffectNum)                = 0;

   // Start a looping background song on the WAV Trigger music channel.
   virtual void playBackgroundSong(unsigned short songNum)             = 0;

   // Select a background song automatically based on ball number (1–n).
   // Cycles through the BACKGROUND_* tracks defined in SoundEffects.h.
   virtual void playBackgroundSongBasedOnBall(uint8_t ballNum)         = 0;

   // Stop all active audio on all WAV Trigger channels.
   virtual void stopAllAudio()                                         {}

   // Play a voice callout on the WAV Trigger notification channel. Callouts
   // duck the FX channel while they play; the notification queue handles
   // priority ordering.
   virtual void playCallout(uint8_t track)                             { (void)track; }

   // Send a command to the native sound card (SB-100, Squawk & Talk, Dash-51).
   // This is separate from the WAV Trigger path; sound = 0 typically silences.
   virtual void playSoundCardEffect(uint8_t sound)                     { (void)sound; }

   // -----------------------------------------------------------------------
   // Credits and coins
   // -----------------------------------------------------------------------

   // Add numToAdd credits to the stored count, optionally playing a coin sound.
   // Writes the new credit count to EEPROM.
   virtual void addCredit(bool playSound = false, uint8_t numToAdd = 1) = 0;

   // Award a credit for a match or special, playing the knocker and updating
   // the replay audit counter.
   virtual void addSpecialCredit()                                     = 0;

   // Increment the coin audit counter for the given chute (1-indexed).
   // Call this whenever a coin is registered, before addCredit.
   virtual void addCoinToAudit(uint8_t chuteNum)                      = 0;

   // Full coin insertion handler: audits the chute and awards credit(s).
   // Returns true if a credit was added. Default returns false (no-op).
   virtual bool addCoin(uint8_t chuteNum)                             { return false; }

   // Engage or release the coin-door lockout coil. Pass true to lock out
   // further coins when credits are at the maximum.
   virtual void setCoinLockout(bool lock)                              { (void)lock; }

   // -----------------------------------------------------------------------
   // Score displays
   //
   // The machine has four 6-digit score displays (0–3) plus a 2-digit
   // credits display and a 2-digit ball-in-play display. All are driven by
   // the RPU display ISR at ~320 Hz.
   //
   // Display index 0 = player 1, 1 = player 2, etc.
   // -----------------------------------------------------------------------

   // Write a static value to a score display.
   //   blankLeadingZeros — suppress leading zeros (e.g. 1234 shows as "  1234")
   //   minimumDigits     — always show at least this many digits from the right
   virtual void setDisplay(uint8_t display, unsigned long value,
                            bool blankLeadingZeros = false,
                            uint8_t minimumDigits  = 0)               { (void)display; (void)value; (void)blankLeadingZeros; (void)minimumDigits; }

   // Write a value to a score display and make it flash at the given period (ms).
   virtual void setDisplayFlash(uint8_t display, unsigned long value,
                                 uint16_t period = 500)               { (void)display; (void)value; (void)period; }

   // Write to the 2-digit credits display. Pass showCredits=false to blank it.
   virtual void setDisplayCredits(uint8_t credits,
                                   bool showCredits = true)           { (void)credits; (void)showCredits; }

   // Write to the 2-digit ball-in-play display. Pass showBall=false to blank it.
   // During attract mode this is repurposed to show the rule-set number.
   virtual void setDisplayBallInPlay(uint8_t ball,
                                      bool showBall = true)           { (void)ball; (void)showBall; }

   // Set the blank mask for a score display directly. Each bit controls one
   // digit; bit 0 = rightmost. Used for selective digit suppression during
   // match sequence and bonus countdown.
   virtual void setDisplayBlank(uint8_t display, uint8_t mask)        { (void)display; (void)mask; }

   // Read the current blank mask for a score display.
   virtual uint8_t getDisplayBlank(uint8_t display)                   { (void)display; return 0; }

   // Cycle all four score displays through a diagnostic sweep at currentTime,
   // showing curValue on each. Used by the hardware self-test.
   virtual void cycleAllDisplays(unsigned long t, uint8_t curValue)   { (void)t; (void)curValue; }

   // -----------------------------------------------------------------------
   // Lamps
   //
   // Lamp numbers are defined in Trident.h (SW_*, LAMP_*, SOL_* constants).
   // The RPU lamp ISR strobes the lamp matrix at ~320 Hz.
   // -----------------------------------------------------------------------

   // Turn off every lamp on the playfield and backglass immediately.
   virtual void turnOffAllLamps()                                      {}

   // Set a single lamp state.
   //   on        — true = illuminate, false = extinguish
   //   dimmer    — 0 = full brightness, 1 = dim (controlled by dimLevel setting)
   //   flashRate — flash period in ms; 0 = steady
   virtual void setLampState(uint8_t lamp, bool on,
                              uint8_t dimmer = 0, uint16_t flashRate = 0) { (void)lamp; (void)on; (void)dimmer; (void)flashRate; }

   // Push a raw lamp-state buffer directly to the RPU lamp animation register.
   // bytes must point to NUM_LAMP_ANIMATION_BYTES of data obtained from
   // PeekAnimationBytes(). Used by AttractMode to drive playfield animations.
   virtual void setLampAnimationBytes(const uint8_t* bytes, uint8_t count) { (void)bytes; (void)count; }

   // Set the dim divisor for dimmer level 1. Higher divisor = dimmer lamps.
   // Typical values: 2 (brighter) or 3 (dimmer). level1 is the slot index.
   virtual void setDimDivisor(uint8_t level1, uint8_t level2)         { (void)level1; (void)level2; }

   // -----------------------------------------------------------------------
   // Solenoids and flippers
   //
   // The RPU solenoid stack is a timed queue serviced by the main loop.
   // duration values are in units of 1/120 s (approx 8.3 ms each).
   // -----------------------------------------------------------------------

   // Prevent any solenoids from firing (e.g. during attract mode or tilt).
   virtual void disableSolenoidStack()                                 {}

   // Re-enable solenoid firing after a disable.
   virtual void enableSolenoidStack()                                  {}

   // Enable or disable both flipper coils. Pass true to kill power to flippers
   // (tilt, end of ball); pass false to restore normal flipper operation.
   virtual void setDisableFlippers(bool disable)                       { (void)disable; }

   // Fire a solenoid immediately (next stack service).
   //   sol             — solenoid number (SOL_* constants in Trident.h)
   //   duration        — hold time in 1/120 s units
   //   disableOverride — if true, fire even when the solenoid stack is disabled
   virtual void pushToSolenoidStack(uint8_t sol, uint8_t duration,
                                     bool disableOverride = false)      { (void)sol; (void)duration; (void)disableOverride; }

   // Schedule a solenoid to fire at a future time.
   //   whenToFire      — millis() timestamp at which to fire
   //   numPushes       — number of times to push to the stack (for multi-fire)
   virtual void pushToTimedSolenoidStack(uint8_t sol, uint8_t numPushes,
                                          unsigned long whenToFire,
                                          bool disableOverride = false)  { (void)sol; (void)numPushes; (void)whenToFire; (void)disableOverride; }

   // -----------------------------------------------------------------------
   // Switches
   //
   // The RPU switch stack is a debounced FIFO filled by the switch ISR.
   // Game logic drains it every loop tick by calling pullFirstFromSwitchStack
   // in a while loop until SWITCH_STACK_EMPTY is returned.
   // -----------------------------------------------------------------------

   // Remove and return the oldest pending switch hit. Returns SWITCH_STACK_EMPTY
   // (0xFF) when the queue is empty.
   virtual uint8_t pullFirstFromSwitchStack()                          { return 0xFF; }

   // Read the instantaneous (debounced) state of a single switch without
   // consuming it from the stack. Useful for checking outhole, saucer, etc.
   virtual bool readSingleSwitchState(uint8_t sw)                      { (void)sw; return false; }

   // Read the up/down switch state (coin-door service buttons on some machines).
   virtual bool getUpDownSwitchState()                                 { return false; }

   // -----------------------------------------------------------------------
   // EEPROM (thin wrappers around RPU_ReadByteFromEEProm etc.)
   //
   // Game code uses these rather than calling EEPROM or RPU directly so that
   // the address space is managed in one place.
   // -----------------------------------------------------------------------

   virtual uint8_t       readByteFromEEProm(uint16_t addr)            { (void)addr; return 0xFF; }
   virtual void          writeByteToEEProm(uint16_t addr, uint8_t val){ (void)addr; (void)val; }
   virtual unsigned long readULFromEEProm(uint16_t addr)              { (void)addr; return 0; }
   virtual void          writeULToEEProm(uint16_t addr, unsigned long val) { (void)addr; (void)val; }

   // -----------------------------------------------------------------------
   // Self-test interlock
   //
   // The self-test switch requires a debounce gap between activations to
   // prevent rapid state changes when the coin-door button is held.
   // getSelfTestChangedTime / setSelfTestChangedTime share the last-fired
   // timestamp across modes so any mode can enforce the debounce window.
   // -----------------------------------------------------------------------

   virtual unsigned long getSelfTestChangedTime()                      { return 0; }
   virtual void          setSelfTestChangedTime(unsigned long t)       { (void)t; }

   // -----------------------------------------------------------------------
   // Miscellaneous
   // -----------------------------------------------------------------------

   // Set the rollover lane value displayed and scored (used by some modes).
   virtual void setRolloverValue(uint8_t v)                           { (void)v; }

   // Called every loop() tick to service audio handlers, timed solenoids,
   // and other machine bookkeeping. Must be called before game-mode update().
   virtual void update(unsigned long currentTime)                     { (void)currentTime; }

   // Re-read all operator settings from EEPROM into the MachineSettings
   // struct. Called on entry to attract mode after the operator exits the
   // adjustment menus.
   virtual void readStoredParameters()                                 {}

   // -----------------------------------------------------------------------
   // Accessors (pure virtual — every concrete subclass must implement these)
   // -----------------------------------------------------------------------

   virtual uint8_t getCredits() const = 0;

   // All-time high score for the Trident 2020 rule set (stored in RPU EEPROM).
   virtual unsigned long getHighScore() const = 0;

   // All-time high score for the Original Trident rule set (game EEPROM block).
   virtual unsigned long getOriginalHighScore() const                  { return 0; }

   virtual bool getFreePlayMode() const = 0;

   // Direct access to the operator-adjustable settings. Returned by reference
   // so callers can read and write fields without extra getters.
   virtual MachineSettings& getSettings() = 0;

   // -----------------------------------------------------------------------
   // Last-game result (stored on the machine between games for attract mode)
   //
   // Trident2020Game calls setLastGameResult() in showPlayerScores() once
   // all scores have been displayed. AttractMode reads the values in enter()
   // so they are stable for the entire attract cycle.
   // -----------------------------------------------------------------------

   // Store the final scores for up to 4 players. scores must point to an
   // array of at least numPlayers elements.
   virtual void setLastGameResult(uint8_t numPlayers, const unsigned long* scores) { (void)numPlayers; (void)scores; }

   // Returns the number of players in the last completed game (0 if none).
   virtual uint8_t getLastGameNumPlayers() const                       { return 0; }

   // Returns the final score for the given player index (0-based).
   virtual unsigned long getLastGameScore(uint8_t player) const        { (void)player; return 0; }
};
