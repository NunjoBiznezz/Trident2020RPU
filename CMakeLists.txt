cmake_minimum_required(VERSION 3.20)

# Set the toolchain file for Arduino before project() call
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ArduinoToolchain.cmake" CACHE FILEPATH "Arduino toolchain file")

project(Trident2020RPU C CXX ASM)

# Build configuration options
# Note: Rev 1 & 2 (Arduino Nano) not supported - Trident2020 requires MEGA 2560 (Rev 3+)
option(RPU_BUILD_FOR_REV3 "Build for RPU hardware revision 3 (MEGA)" OFF)
option(RPU_BUILD_FOR_REV4 "Build for RPU hardware revision 4 (MEGA)" ON)
option(RPU_BUILD_FOR_REV102 "Build for RPU hardware revision 102 (MEGA)" OFF)
option(ENABLE_DEBUG_MESSAGES "Enable debug messages over serial" OFF)

# RPU feature options
option(RPU_USE_DIP_SWITCHES "Enable DIP switch reading" ON)
option(RPU_USE_SB100 "Enable SB-100 sound card support" ON)
option(RPU_USE_SB300 "Enable SB-300 sound card support" ON)
option(RPU_USE_WAV_TRIGGER "Enable WavTrigger 1.3 support" ON)
option(RPU_USE_7_DIGIT_DISPLAYS "Enable 7-digit displays" OFF)
option(RPU_USE_6_DIGIT_CREDIT_WITH_7_DIGIT "Use 6-digit credit display with 7-digit displays" OFF)
option(RPU_USE_AUX_LAMPS "Enable auxiliary lamps" OFF)

# Determine board type based on revision (all supported revisions use MEGA)
set(ARDUINO_BOARD "megaatmega2560")
if(RPU_BUILD_FOR_REV3)
    set(RPU_HARDWARE_REV 3)
elseif(RPU_BUILD_FOR_REV102)
    set(RPU_HARDWARE_REV 102)
else()
    set(RPU_HARDWARE_REV 4)
endif()

# Arduino core library
find_package(Arduino REQUIRED)

# Main executable target
set(SOURCES
    src/main.cpp
    src/AudioHandler.cpp
    src/WavTriggerHandler.cpp
    src/SelfTestAndAudit.cpp
)

set(HEADERS
    include/AudioHandler.h
    include/MachineState.h
    include/SelfTestAndAudit.h
    include/SoundEffects.h
    include/Trident2020.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Build flags matching platformio.ini
target_compile_definitions(${PROJECT_NAME} PRIVATE
    RPU_OS_HARDWARE_REV=${RPU_HARDWARE_REV}
    RPU_MPU_BUILD_FOR_6800=1
)

# Feature flags (passed through from options)
if(RPU_USE_DIP_SWITCHES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RPU_OS_USE_DIP_SWITCHES)
endif()
if(RPU_USE_SB100)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RPU_OS_USE_SB100)
endif()
if(RPU_USE_SB300)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RPU_OS_USE_SB300)
endif()
if(RPU_USE_WAV_TRIGGER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RPU_OS_USE_WAV_TRIGGER)
endif()

if(RPU_BUILD_FOR_REV102 AND ENABLE_DEBUG_MESSAGES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        DEBUG_MESSAGES
    )
endif()

# Pass required architecture configuration to RPU library
set(RPU_OS_HARDWARE_REV ${RPU_HARDWARE_REV} CACHE STRING "" FORCE)
set(RPU_MPU_BUILD_FOR_6800 1 CACHE STRING "" FORCE)

# Pass feature flags to RPU library
set(RPU_ENABLE_DIP_SWITCHES ${RPU_USE_DIP_SWITCHES} CACHE BOOL "" FORCE)
set(RPU_ENABLE_SB100 ${RPU_USE_SB100} CACHE BOOL "" FORCE)
set(RPU_ENABLE_SB300 ${RPU_USE_SB300} CACHE BOOL "" FORCE)
set(RPU_ENABLE_WAV_TRIGGER ${RPU_USE_WAV_TRIGGER} CACHE BOOL "" FORCE)
set(RPU_ENABLE_7_DIGIT_DISPLAYS ${RPU_USE_7_DIGIT_DISPLAYS} CACHE BOOL "" FORCE)
set(RPU_ENABLE_6_DIGIT_CREDIT_WITH_7_DIGIT ${RPU_USE_6_DIGIT_CREDIT_WITH_7_DIGIT} CACHE BOOL "" FORCE)
set(RPU_ENABLE_AUX_LAMPS ${RPU_USE_AUX_LAMPS} CACHE BOOL "" FORCE)

# Add RPU library
add_subdirectory(lib/RPU)
target_link_libraries(${PROJECT_NAME} PRIVATE RPU)

# Add WavTrigger library
add_subdirectory(lib/WavTrigger)
target_link_libraries(${PROJECT_NAME} PRIVATE WavTrigger)

# Arduino core and standard libraries (all supported boards are MEGA 2560)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Arduino::Core
    Arduino::EEPROM
    Arduino::HardwareSerial
)

# Configure upload settings
set_target_properties(${PROJECT_NAME} PROPERTIES
    ARDUINO_BOARD ${ARDUINO_BOARD}
    ARDUINO_PORT AUTO
)

# Print configuration summary
message(STATUS "=== Trident2020RPU Build Configuration ===")
message(STATUS "Hardware Revision: ${RPU_HARDWARE_REV}")
message(STATUS "Target Board: ${ARDUINO_BOARD}")
message(STATUS "Debug Messages: ${ENABLE_DEBUG_MESSAGES}")
message(STATUS "==========================================")
