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
```

**CMake (secondary — useful for IDE integration):**
```bash
cmake --preset rev4                  # configure (rev3, rev4, rev102)
cmake --build --preset rev4
```

## Hardware revisions

All supported revisions target the Arduino MEGA 2560. Select the right one via `-e` in PlatformIO or `--preset` in CMake:

| Rev | Board | Notes                                    |
|-----|-------|------------------------------------------|
| 3 | MEGA 2560 Pro | WAV Trigger on `Serial` (with RX added)  |
| 4 | MEGA 2560 Pro (larger) | WAV Trigger on `Serial1` (bidirectional) |
| 102 | MEGA 2560 w/ display+WiFi | Enables `DEBUG_MESSAGES`                 |

Rev 3 vs Rev 4 matters for audio: `WavTrigger::hasSerialRx` is false on Rev 3, which disables sound-effect ducking during callouts (`duckCurrentSoundEffects()` returns early).

## Feature flags

All `RPU_OS_USE_*` defines are set by `platformio.ini` or CMake options — never hardcode them in source. Key ones:

- `RPU_OS_USE_WAV_TRIGGER` — enables WAV Trigger audio path
- `RPU_OS_USE_SB100` / `RPU_OS_USE_SB300` — original Stern sound card paths
- `RPU_OS_USE_DIP_SWITCHES` — read physical DIP switches at boot

## Architecture

### Layers (bottom to top)

**`lib/RPU/`** — Hardware abstraction for the vintage pinball MPU bus. Provides interrupt-driven lamp strobing, display multiplexing, solenoid firing, and switch debouncing. Uses `CircularQueue` for the solenoid and switch queues. Game code calls `RPU_*` functions — it never touches hardware registers directly.

**`lib/WavTrigger/`** — Serial driver for the WAV Trigger board.

**`src/AudioHandler.cpp`** — Three independent audio channels (sound FX, background music, voice notifications) unified over whatever backends are compiled in. Called once per loop via `audioHandler.update(CurrentTime)`.

**`src/SelfTestAndAudit.cpp`** — Coin-door button menu: hardware tests (lamps, displays, solenoids, switches, sounds) and 29 operator-adjustable settings. Runs when `MachineState < 0`.

**`src/main.cpp`** — All game logic for Trident 2020. Single-file Arduino sketch structure with `setup()` / `loop()` at the bottom and all game state as file-static globals.

### Main loop pattern

```cpp
// Every loop() tick:
CurrentTime = millis();
// ...service switches, audio, solenoids via RPU...
if (MachineState < 0)      RunSelfTest(...);
else if (MachineState == 0) RunAttractMode(...);
else                        RunGamePlay(...);
```

`MachineStateChanged = true` triggers one-time initialization on the next tick after a state transition.

### Machine state values

- **Negative** — self-test / operator adjust (defined in `SelfTestAndAudit.h`)
- **0** — attract mode
- **1–4** — game play phases (`INIT_GAMEPLAY`, `INIT_NEW_BALL`, `NORMAL_GAMEPLAY`)
- **99–110** — end-of-ball sequence (`COUNTDOWN_BONUS`, `BALL_OVER`, `MATCH_MODE`)

### Game modes (within normal gameplay)

Stored in the `GameMode` byte using flags:

- `GAME_MODE_SKILL_SHOT` (0), `UNSTRUCTURED_PLAY` (1), `MINI_GAME_*` (2–4)
- `GAME_MODE_FEEDING_FRENZY_FLAG` (0x10), `SHARP_SHOOTER_FLAG` (0x20), `EXPLORE_THE_DEPTHS_FLAG` (0x40)
- `GAME_MODE_WIZARD` (0x7F) — all three mini-games active simultaneously

### EEPROM layout

Settings are stored at fixed byte offsets 100–144 in EEPROM. The constants are defined at the top of `main.cpp` (`EEPROM_*_BYTE`). Use `ReadSetting(offset, default)` to read; write via `EEPROM.write()`.

### Audio

`AudioHandler` wraps all audio backends. Three volume channels, each settable 0–10:
- **Sound FX** — one-shot effects via `PlaySoundEffect()` → `audioHandler.playSound()`
- **Background music** — looping track or auto-advancing soundtrack via `audioHandler.playBackgroundSong()`
- **Notifications** (voice callouts) — FIFO queue with priority via `audioHandler.queuePrioritizedNotification()`; music and FX auto-duck while a notification plays

Sound effect numbers are defined in `SoundEffects.h`. WAV files on the SD card are referenced by track number = sound effect number.

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
