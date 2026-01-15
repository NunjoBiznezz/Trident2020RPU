# CMake Build Instructions

This project now supports CMake in addition to PlatformIO for building the Trident 2020 RPU controller.

## Prerequisites

### Option 1: Using PlatformIO (Recommended for Arduino projects)
PlatformIO handles the Arduino toolchain automatically:
```bash
# Install PlatformIO Core
pip install platformio

# PlatformIO can generate CMake files for IDE integration
pio project init --ide cmake
```

### Option 2: Using arduino-cmake
For native CMake builds, install the arduino-cmake toolchain:
```bash
# Clone arduino-cmake
git clone https://github.com/arduino-cmake/arduino-cmake.git

# Update cmake/ArduinoToolchain.cmake to point to your arduino-cmake installation
```

## Building with CMake

### Using CMake Presets (Recommended)

The project includes CMake presets for each hardware revision:

```bash
# List available presets
cmake --list-presets

# Configure for specific hardware revision
cmake --preset rev4          # Default: MEGA 2560 Rev 4
cmake --preset rev3          # MEGA 2560 Rev 3
cmake --preset rev102        # MEGA 2560 Rev 102 with debug
cmake --preset rev1          # Nano Rev 1
cmake --preset rev2          # Nano Rev 2

# Build
cmake --build --preset rev4

# Or configure and build in one step
cmake --workflow --preset rev4
```

### Manual Configuration

```bash
# Create build directory
mkdir build && cd build

# Configure for Rev 4 (default)
cmake ..

# Configure for specific revision
cmake .. -DRPU_BUILD_FOR_REV3=ON -DRPU_BUILD_FOR_REV4=OFF

# Enable debug messages (Rev 102)
cmake .. -DRPU_BUILD_FOR_REV102=ON -DENABLE_DEBUG_MESSAGES=ON

# Build
cmake --build .
```

## Hardware Revision Options

- `RPU_BUILD_FOR_REV1`: Hardware Rev 1 (Arduino Nano ATmega328)
- `RPU_BUILD_FOR_REV2`: Hardware Rev 2 (Arduino Nano ATmega328)
- `RPU_BUILD_FOR_REV3`: Hardware Rev 3 (Arduino MEGA 2560)
- `RPU_BUILD_FOR_REV4`: Hardware Rev 4 (Arduino MEGA 2560) - **Default**
- `RPU_BUILD_FOR_REV102`: Hardware Rev 102 (Arduino MEGA 2560)
- `ENABLE_DEBUG_MESSAGES`: Enable serial debug output

## Library Modules

The CMake build automatically includes:
- **RPU Library** (`lib/RPU/`): Hardware abstraction for vintage pinball MPUs
- **WavTrigger Library** (`lib/WavTrigger/`): Audio playback via WAV Trigger board

Each library has its own `CMakeLists.txt` for modular building.

## IDE Integration

### VS Code
Install the CMake Tools extension:
```bash
code --install-extension ms-vscode.cmake-tools
```

The project includes `CMakePresets.json` which VS Code will detect automatically.
Select a preset from the status bar to configure and build.

### CLion
CLion has native CMake support. Open the project and it will detect the CMake configuration automatically.

## Build Outputs

Compiled binaries are placed in:
```
build/<preset-name>/Trident2020RPU.elf
build/<preset-name>/Trident2020RPU.hex
```

## Comparison with PlatformIO

| Feature | PlatformIO | CMake |
|---------|-----------|-------|
| Arduino toolchain | Automatic | Manual setup required |
| Dependencies | Automatic | Manual configuration |
| Upload to board | `pio run -t upload` | Requires avrdude setup |
| IDE support | Good | Excellent (CLion, VS Code) |
| Build speed | Moderate | Fast (with Ninja) |
| Learning curve | Low | Moderate |

**Recommendation**: Use PlatformIO for quick development and flashing. Use CMake for IDE integration and when working on the RPU/WavTrigger libraries.

## Troubleshooting

### Arduino SDK not found
Set the `ARDUINO_SDK_PATH` variable:
```bash
cmake .. -DARDUINO_SDK_PATH=/path/to/arduino
```

### Missing arduino-cmake
The project includes a placeholder toolchain. Install arduino-cmake and update `cmake/ArduinoToolchain.cmake`.

### Library linking errors
Ensure all submodules are initialized and the RPU/WavTrigger libraries are present in `lib/`.
