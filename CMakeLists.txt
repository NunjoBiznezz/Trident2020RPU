cmake_minimum_required(VERSION 3.20)

# Set the toolchain file for Arduino before project() call
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ArduinoToolchain.cmake" CACHE FILEPATH "Arduino toolchain file")

project(Trident2020RPU C CXX ASM)

# Build configuration options
# Note: Rev 1 & 2 (Arduino Nano) not supported - Trident2020 requires MEGA 2560 (Rev 3+)
option(RPU_BUILD_FOR_REV3   "Build for RPU hardware revision 3 (MEGA)"   OFF)
option(RPU_BUILD_FOR_REV4   "Build for RPU hardware revision 4 (MEGA)"   ON)
option(RPU_BUILD_FOR_REV102 "Build for RPU hardware revision 102 (MEGA)" OFF)
# Debug output — all disabled by default.
# Set DEBUG_PORT to the Arduino HardwareSerial object to use (e.g. Serial, Serial1, Serial2).
# Leave empty to disable all debug output; the message flags below have no effect without it.
# Rev 3 uses Serial for WAV Trigger — use Serial2 or higher on that rev.
# Rev 4+ use Serial1 for WAV Trigger — Serial is free.
set(DEBUG_PORT "" CACHE STRING "Serial port for debug output (e.g. Serial2). Empty = all debug disabled.")
option(DEBUG_GAME_MESSAGES "Enable game-code debug messages (requires DEBUG_PORT)" OFF)
option(DEBUG_RPU_MESSAGES  "Enable RPU library debug messages (requires DEBUG_PORT)" OFF)

# RPU feature options — exactly one native sound card may be enabled per build
option(RPU_USE_DIP_SWITCHES          "Enable DIP switch reading"                     ON)
option(RPU_USE_SB100                 "Enable SB-100 sound card support"              OFF)
option(RPU_USE_SB300                 "Enable SB-300 sound card support"              OFF)
option(RPU_USE_S_AND_T               "Enable S&T sound card support"                 OFF)
option(RPU_USE_DASH51                "Enable Dash-51 sound card support"             OFF)
option(RPU_USE_WAV_TRIGGER           "Enable WavTrigger 1.3 support"                 ON)
option(RPU_USE_7_DIGIT_DISPLAYS      "Enable 7-digit displays"                       OFF)
option(RPU_USE_6_DIGIT_CREDIT_WITH_7_DIGIT "Use 6-digit credit display with 7-digit displays" OFF)
option(RPU_USE_AUX_LAMPS             "Enable auxiliary lamps"                        OFF)

# Determine hardware revision (all supported revisions use MEGA 2560)
set(ARDUINO_BOARD "megaatmega2560")
if(RPU_BUILD_FOR_REV3)
    set(RPU_HARDWARE_REV 3)
elseif(RPU_BUILD_FOR_REV102)
    set(RPU_HARDWARE_REV 102)
else()
    set(RPU_HARDWARE_REV 4)
endif()

# Pass configuration to the RPU library (must be set before add_subdirectory)
set(RPU_OS_HARDWARE_REV    ${RPU_HARDWARE_REV} CACHE STRING "" FORCE)
set(RPU_MPU_BUILD_FOR_6800 1                   CACHE STRING "" FORCE)

set(RPU_ENABLE_DIP_SWITCHES          ${RPU_USE_DIP_SWITCHES}          CACHE BOOL "" FORCE)
set(RPU_ENABLE_SB100                 ${RPU_USE_SB100}                 CACHE BOOL "" FORCE)
set(RPU_ENABLE_SB300                 ${RPU_USE_SB300}                 CACHE BOOL "" FORCE)
set(RPU_ENABLE_S_AND_T               ${RPU_USE_S_AND_T}               CACHE BOOL "" FORCE)
set(RPU_ENABLE_DASH51                ${RPU_USE_DASH51}                CACHE BOOL "" FORCE)
set(RPU_ENABLE_WAV_TRIGGER           ${RPU_USE_WAV_TRIGGER}           CACHE BOOL "" FORCE)
set(RPU_ENABLE_7_DIGIT_DISPLAYS      ${RPU_USE_7_DIGIT_DISPLAYS}      CACHE BOOL "" FORCE)
set(RPU_ENABLE_6_DIGIT_CREDIT_WITH_7_DIGIT ${RPU_USE_6_DIGIT_CREDIT_WITH_7_DIGIT} CACHE BOOL "" FORCE)
set(RPU_ENABLE_AUX_LAMPS             ${RPU_USE_AUX_LAMPS}             CACHE BOOL "" FORCE)

# Debug defines — applied before add_subdirectory so RPU library sees them too.
# add_compile_definitions() is directory-scoped: it propagates to all subdirectories.
if(DEBUG_PORT)
    add_compile_definitions("DEBUG_PORT=${DEBUG_PORT}")
    if(DEBUG_GAME_MESSAGES)
        add_compile_definitions(DEBUG_MESSAGES)
    endif()
    if(DEBUG_RPU_MESSAGES)
        add_compile_definitions(RPU_DEBUG_MESSAGES)
    endif()
endif()

# Arduino core library
find_package(Arduino REQUIRED)

# Libraries (must come before the game target so PUBLIC defines propagate)
add_subdirectory(lib/RPU)
add_subdirectory(lib/WavTrigger)

# Game executable — only game-specific sources
set(SOURCES
    src/main.cpp
    src/AudioHandler.cpp
    src/WavTriggerHandler.cpp
)

set(HEADERS
    include/AudioHandler.h
    include/WavTriggerHandler.h
    include/MachineState.h
    include/SoundEffects.h
    include/Trident2020.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Game-specific include path (RPU headers come via the RPU library's PUBLIC includes)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE RPU WavTrigger)

# Configure upload settings
set_target_properties(${PROJECT_NAME} PROPERTIES
    ARDUINO_BOARD ${ARDUINO_BOARD}
    ARDUINO_PORT AUTO
)

message(STATUS "=== Trident2020RPU Build Configuration ===")
message(STATUS "Hardware Revision : ${RPU_HARDWARE_REV}")
message(STATUS "Target Board      : ${ARDUINO_BOARD}")
message(STATUS "WAV Trigger       : ${RPU_USE_WAV_TRIGGER}")
if(DEBUG_PORT)
    message(STATUS "Debug Port        : ${DEBUG_PORT}")
    message(STATUS "Game debug        : ${DEBUG_GAME_MESSAGES}")
    message(STATUS "RPU debug         : ${DEBUG_RPU_MESSAGES}")
else()
    message(STATUS "Debug             : disabled (set DEBUG_PORT to enable)")
endif()
message(STATUS "==========================================")
