# Trident 2020 RPU - AI Coding Instructions

## Project Overview
Trident 2020 is a single-ball pinball machine controller running on Arduino MEGA 2560 (rev 3+). This replaces the original MPU board with modern hardware while maintaining compatibility with vintage Bally/Stern (6800-based) pinball hardware through the custom RPU (Retro Pinball Unit) interface library.

## Architecture

### Core Components
- **RPU Library** (`lib/RPU/`): Hardware abstraction layer for vintage pinball MPU boards (-17, -35, 100, 200 architectures). Handles low-level PIA chip communication, display multiplexing, switch/lamp matrices, and solenoid control
- **Main Game Logic** (`src/main.cpp`): 2900+ line state machine managing game modes, scoring, and player progression
- **AudioHandler** (`src/AudioHandler.cpp`, `include/AudioHandler.h`): Unified audio system supporting both original sound card commands and WAV Trigger board. Manages ducking, priorities, and queued playback. Must call `Audio.Update(CurrentTime)` every loop iteration
- **Hardware Definitions** (`include/Trident2020.h`): Game-specific mappings of switches (SW_*), solenoids (SOL_*), and lamps using `constexpr` for zero runtime overhead

### State Machine Pattern
The codebase uses two overlapping state machines:
1. **MachineState** (in `include/MachineState.h`): Top-level states (attract mode, gameplay, self-test). Negative values = self-test modes, 0 = attract, positive = gameplay states
2. **GameMode** (in `src/main.cpp`): Gameplay sub-states like `GAME_MODE_SKILL_SHOT`, `GAME_MODE_UNSTRUCTURED_PLAY`, `GAME_MODE_MINI_GAME_ENGAGED`, and wizard modes with bitwise flags (e.g., `GAME_MODE_SHARP_SHOOTER_FLAG`)

Game modes use bitwise flags for composite states - check with `(GameMode & FLAG)` not `(GameMode == FLAG)` for wizard variants.

### Hardware Communication
- All hardware I/O goes through RPU library - never write directly to ports
- Switch reads are debounced and pushed to a stack; poll with `RPU_PullFirstFromSwitchStack()`
- Solenoid fires use `RPU_PushToSolenoidStack(solenoidNum, numPushes)` with automatic timing
- Lamps controlled via `RPU_SetLampState(lampNum, on/off, dim, flashing)`
- Display updates happen automatically via interrupt - just call `RPU_SetDisplay(playerNum, score)`

## Build System

This project supports both PlatformIO and CMake. **PlatformIO is recommended** for quick development and flashing to hardware.

### PlatformIO Build Environments
Five build environments in [platformio.ini](../platformio.ini) for different RPU hardware revisions:
- **rev3, rev4**: Arduino MEGA 2560 (standard and with display/WIFI)
- **rev100, rev101, rev102**: CPU socket interposer variants
- Rev 1-2 (Arduino Nano) are commented out - Trident2020 requires MEGA 2560

Key configuration:
- `platform = atmelavr`, `board = megaatmega2560`, `framework = arduino`
- Build flags set in `build_flags` control hardware features via compile-time defines

### Critical Build Flags
```ini
-DRPU_OS_HARDWARE_REV=4        # Hardware revision number
-DRPU_MPU_ARCHITECTURE=1       # Bally/Stern -17/-35 architecture
-DRPU_MPU_BUILD_FOR_6800=1     # Target 6800 CPU (vs 6802/6808)
-DRPU_OS_USE_DIP_SWITCHES      # Enable DIP switch reading
-DRPU_OS_USE_SB100             # Enable SB-100 sound card support
-DDEBUG_MESSAGES               # Enable serial debug output
```

### Build Commands
```bash
# Build for specific hardware revision
pio run -e rpu_os_hardware_rev4

# Upload to board
pio run -e rpu_os_hardware_rev4 -t upload

# Monitor serial output (57600 baud for WAV Trigger, 115200 for debug)
pio device monitor -b 115200
```

### CMake Build (Alternative)
CMake support is available for IDE integration and library development. See [README_CMAKE.md](../README_CMAKE.md) for details.

```bash
# Using presets
cmake --preset rev4          # Configure for Rev 4
cmake --build --preset rev4  # Build

# Manual configuration
cmake -B build -DRPU_BUILD_FOR_REV4=ON
cmake --build build
```

**Note:** PlatformIO handles Arduino toolchain and dependencies automatically. CMake requires manual setup but offers better IDE integration (CLion, VS Code CMake Tools).

## Audio System

The `AudioHandler` class abstracts multiple audio backends:
- **WAV Trigger**: SD card-based audio playback (preferred, activated via `AUDIO_PLAY_TYPE_WAV_TRIGGER`)
- **Original Sound Card**: SB-100/SB-300 commands through RPU interface (`AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS`)

Audio files referenced in [README.md](../README.md) Google Drive link. The system supports:
- Volume control per category (music, SFX, callouts) via 0-10 scale
- Automatic music ducking during voice callouts
- Priority-based notification queueing
- Future-timed sound queuing with `QueueSound(soundIndex, audioType, playTime)`

**Critical:** Always call `Audio.Update(CurrentTime)` in the main loop. Example usage:
```cpp
// Setup
Audio.InitDevices(AUDIO_PLAY_TYPE_WAV_TRIGGER | AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS);
Audio.SetMusicDuckingGain(16);  // Gain reduction for music during callouts

// During gameplay
Audio.PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1, true);  // Loop background track
Audio.PlaySound(soundNum, AUDIO_PLAY_TYPE_WAV_TRIGGER);     // Play sound effect
Audio.QueueSound(0x02, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);  // Queue sound card command

// In loop()
Audio.Update(CurrentTime);  // Required for queued sounds, ducking, and notifications
```

## Code Conventions

### Naming Patterns
- **Hardware constants**: `SW_*` (switches), `SOL_*` (solenoids), lamp numbers as bare constants (e.g., `BONUS_1`)
- **Game state constants**: `GAME_MODE_*`, `MACHINE_STATE_*`
- **EEPROM addresses**: `EEPROM_*_BYTE` 
- **Timing values**: Suffix with `_TIME` or `_DURATION` in milliseconds (e.g., `MODE_QUALIFY_TIME`)
- **Masks for bitwise flags**: Suffix with `_MASK` (e.g., `STANDUP_PURPLE_MASK`)

### Modern C++ Usage
- Prefer `constexpr` over `#define` for constants - it's type-safe and scoped
- Use `uint8_t`, `uint16_t`, `unsigned long` for explicit sizing (Arduino convention)
- `constexpr` calculations happen at compile-time, saving flash and RAM

### Memory Management
This runs on ATmega2560 with limited RAM. Guidelines:
- Store strings in PROGMEM when possible
- Use `uint8_t` instead of `int` for counters that never exceed 255
- Avoid dynamic allocation (`new`, `malloc`) - preallocate arrays
- The display interrupt runs at ~320 Hz - keep interrupt-related code minimal

## Settings and Configuration

Settings stored in EEPROM (see `EEPROM_*_BYTE` constants in [main.cpp](../src/main.cpp#L74-L87)). Access via self-test menu (credit/reset button behind coin door):
- Tests 1-5: Lamp, display, solenoid, switch, sound tests
- Settings 1-29: Audits and adjustable game parameters (see [README.md](../README.md#L9-L37))

Read settings with `ReadSetting(setting, defaultValue)` - never hardcode values that should be configurable.

## Common Tasks

### Adding a New Lamp Effect
1. Define lamp constant in [Trident2020.h](../include/Trident2020.h) if not present
2. Use `RPU_SetLampState(lampNum, 1)` to turn on, `RPU_SetLampState(lampNum, 0, dim)` for dimmed
3. Lamp states are buffered - updates happen automatically during display refresh

### Adding a Switch Handler
1. Add switch constant to [Trident2020.h](../include/Trident2020.h) if not present
2. In main loop, poll switches: `uint8_t switchHit = RPU_PullFirstFromSwitchStack()`
3. Check for your switch: `if (switchHit == SW_YOUR_SWITCH) { ... }`
4. For auto-triggered solenoids (slings, bumpers), add to `TriggeredSwitches[]` array

### Modifying Game Modes
1. Game mode transitions happen by setting `GameMode = NEW_MODE` and `GameModeStartTime = CurrentTime`
2. Use `GameModeEndTime` for timed modes, check with `CurrentTime < GameModeEndTime`
3. For composite wizard modes, use bitwise OR: `GameMode |= GAME_MODE_FEEDING_FRENZY_FLAG`

## Key Files Reference
- [platformio.ini](../platformio.ini) - Build configurations for each hardware revision
- [src/main.cpp](../src/main.cpp) - Main game logic, setup(), loop()
- [include/Trident2020.h](../include/Trident2020.h) - All hardware number definitions
- [include/MachineState.h](../include/MachineState.h) - Machine state constants
- [lib/RPU/RPU.h](../lib/RPU/RPU.h) - RPU library API documentation
- [lib/RPU/RPU_config.h](../lib/RPU/RPU_config.h) - Hardware configuration options

## License
GPL v3 - code is freely available in perpetuity. Original author Dick Hamill disclaimed copyright to encourage community use and modification.
