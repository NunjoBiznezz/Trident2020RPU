# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A replacement game ROM for a Gottlieb **Trident (1979)** pinball machine. An Arduino MEGA 2560 (on an RPU board) replaces the original 6800-based MPU. Audio is handled by a WAV Trigger board over serial. There are no tests — correctness is verified by running on hardware.

## Build commands

**PlatformIO (primary — handles toolchain automatically):**
```bash
pio run                              # build (default: rev3)
pio run -e rpu_os_hardware_rev4      # build for specific hardware rev
pio run -t upload                    # compile and flash to board
pio run -t upload -e rpu_os_hardware_rev4
pio device monitor -b 115200         # monitor serial debug output
```

**CMake (secondary — useful for IDE integration):**
```bash
cmake --preset rev4                  # configure (rev3, rev4, rev102)
cmake --build --preset rev4
```

## Hardware revisions

All supported revisions target the Arduino MEGA 2560. Select the right one via `-e` in PlatformIO or `--preset` in CMake:

| Env | Board | Notes |
|-----|-------|-------|
| `rpu_os_hardware_rev3` | MEGA 2560 Pro | WAV Trigger on `Serial` (RX added) |
| `rpu_os_hardware_rev4` | MEGA 2560 Pro (larger) | WAV Trigger on `Serial1` (bidirectional) |
| `rpu_os_hardware_rev102` | MEGA 2560 w/ display+WiFi | Enables `DEBUG_MESSAGES` |
| `dash51` | MEGA 2560 | Stub env to test Dash-51 sound card |
| `sb100` / `sb300` | MEGA 2560 | Stub envs to test original sound cards |

Rev 3 vs Rev 4 matters for audio: `WavTrigger::hasSerialRx` is false on Rev 3, which disables sound-effect ducking during callouts (`duckCurrentSoundEffects()` returns early).

## Feature flags

All `RPU_OS_USE_*` defines are set by `platformio.ini` or CMake options — never hardcode them in source. Key ones:

- `RPU_OS_USE_WAV_TRIGGER` — enables WAV Trigger audio path
- `RPU_OS_USE_SB100` / `RPU_OS_USE_SB300` — original Stern sound card paths
- `RPU_OS_USE_DIP_SWITCHES` — read physical DIP switches at boot
- `DEBUG_MESSAGES` — enables `DEBUG_MESSAGE(x)` macro → `Serial.write(x)`

## Architecture

### Layers (bottom to top)

**`lib/RPU/`** — Hardware abstraction for the vintage pinball MPU bus. Provides interrupt-driven lamp strobing, display multiplexing, solenoid firing, and switch debouncing. Uses `CircularQueue` for the solenoid and switch queues. Game code calls `RPU_*` functions — it never touches hardware registers directly.

**`lib/WavTrigger/`** — Serial driver for the WAV Trigger board.

**`src/AudioHandler.cpp`** — Manages native sound card output (SB-100, SB-300, Dash-51, Squawk & Talk) via a timed queue. Does **not** handle WAV Trigger. Called once per loop via `audioHandler.update(CurrentTime)`.

**`src/WavTriggerHandler.cpp`** — Owns the WAV Trigger driver. Three independent audio channels (sound FX, background music, voice notifications) with volume control 0–10 per channel, automatic music/FX ducking during callouts, and a priority `NotificationQueue`. Called once per loop via `wavHandler.update(CurrentTime)`.

**`src/SelfTestAndAudit.cpp`** — Coin-door button menu: hardware tests (lamps, displays, solenoids, switches, sounds) and 29 operator-adjustable settings. Runs when `MachineState < 0`.

**`src/main.cpp`** — All game logic for Trident 2020. Single-file Arduino sketch structure with `setup()` / `loop()` at the bottom and all game state as file-static globals.

### Main loop pattern

```cpp
// Every loop() tick:
CurrentTime = millis();
// ...service switches, audio, solenoids via RPU...
if (MachineState < 0)       RunSelfTest(...);
else if (MachineState == 0) RunAttractMode(...);
else                        RunGamePlay(...);
```

`MachineStateChanged = true` triggers one-time initialization on the next tick after a state transition.

### Machine state values

- **Negative** — self-test / operator adjust (defined in `SelfTestAndAudit.h`, accessed via `MachineState.h`)
- **0** — attract mode
- **1–4** — game play phases (`INIT_GAMEPLAY`, `INIT_NEW_BALL`, `NORMAL_GAMEPLAY`)
- **99–110** — end-of-ball sequence (`COUNTDOWN_BONUS`, `BALL_OVER`, `MATCH_MODE`)

### Game modes (within normal gameplay)

Stored in the `GameMode` byte using flags. **Always check with `(GameMode & FLAG)`, not `(GameMode == FLAG)`** — wizard mode sets multiple flags simultaneously.

- `GAME_MODE_SKILL_SHOT` (0), `UNSTRUCTURED_PLAY` (1), `MINI_GAME_*` (2–4)
- `GAME_MODE_FEEDING_FRENZY_FLAG` (0x10), `SHARP_SHOOTER_FLAG` (0x20), `EXPLORE_THE_DEPTHS_FLAG` (0x40)
- `GAME_MODE_WIZARD` (0x7F) — all three mini-games active simultaneously

### EEPROM layout

Settings are stored at fixed byte offsets 100–144 in EEPROM. Constants defined at the top of `main.cpp` (`EEPROM_*_BYTE`). Use `ReadSetting(offset, default)` to read; write via `EEPROM.write()`.

### Audio

Two handlers run in parallel each loop:

- `audioHandler.update(CurrentTime)` — fires time-queued native sound card commands
- `wavHandler.update(CurrentTime)` — manages WAV Trigger: ducking, notification queue, soundtrack advancement

Sound effect numbers are defined in `SoundEffects.h`. WAV files on the SD card are referenced by track number = sound effect number.

For WAV Trigger audio: `wavHandler.playSound()` for one-shot FX, `wavHandler.playBackgroundSong()` for looping music, `wavHandler.queuePrioritizedNotification()` for voice callouts.

### Data structure hierarchy

Custom queue classes in `include/`:

```
SimpleQueue<T, SIZE>          — generic index-based FIFO (no sentinel required)
└── NotificationQueue<SIZE>   — extends SimpleQueue; adds priority scanning,
                                selective invalidation, skip-invalid pull()

CircularQueue<T, SIZE, EMPTY> — sentinel-based FIFO, volatile members for ISR safety
                                (used inside RPU for solenoid/switch queues)
```

`SimpleQueue` uses the standard "one wasted slot" convention: capacity is `SIZE - 1`.

`NotificationQueue` uses `INVALID_NOTIFICATION = 0xFFFF` as a tombstone: `clearUpToPriority()` marks entries in-place; `pull()` skips tombstoned entries automatically.

`CircularStack.h` and `SimpleStack.h` are redirect shims to their renamed counterparts — do not use them for new code.

## Code conventions

### Naming patterns

- **Hardware constants**: `SW_*` (switches), `SOL_*` (solenoids), lamp numbers as bare `constexpr` (e.g., `BONUS_1`)
- **Game state constants**: `GAME_MODE_*`, `MACHINE_STATE_*`
- **EEPROM addresses**: `EEPROM_*_BYTE`
- **Timing values**: suffix with `_TIME` or `_DURATION`, in milliseconds
- **Bitwise masks**: suffix with `_MASK` (e.g., `STANDUP_PURPLE_MASK`)

### Adding auto-triggered solenoids

Pop bumpers and slings are wired in `TriggeredSwitches[]` in `Trident2020.h` — they fire their solenoid automatically without game-loop switch handling. The array entry specifies `{switchNum, solenoidNum, durationIn120ths}`.

### Memory constraints (ATmega2560)

- Prefer `constexpr` over `#define` — type-safe and avoids macro pitfalls
- Use `uint8_t`/`uint16_t`/`unsigned long` for explicit sizing
- Store string literals in PROGMEM when possible
- Never use dynamic allocation (`new`, `malloc`) — preallocate all arrays
- The display interrupt runs at ~320 Hz — keep ISR-adjacent code minimal

### Formatting

The project uses clang-format with 3-space indentation, 140-column limit, LLVM style with `InsertBraces: true`. Run `clang-format -i <file>` to auto-format. clang-tidy is configured for readability and bugprone checks (see `.clang-tidy`).
