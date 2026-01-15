cmake_minimum_required(VERSION 3.20)

# Set the toolchain file for Arduino before project() call
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ArduinoToolchain.cmake" CACHE FILEPATH "Arduino toolchain file")

project(Trident2020RPU C CXX ASM)

# Build configuration options
option(RPU_BUILD_FOR_REV1 "Build for RPU hardware revision 1 (Nano)" OFF)
option(RPU_BUILD_FOR_REV2 "Build for RPU hardware revision 2 (Nano)" OFF)
option(RPU_BUILD_FOR_REV3 "Build for RPU hardware revision 3 (MEGA)" OFF)
option(RPU_BUILD_FOR_REV4 "Build for RPU hardware revision 4 (MEGA)" ON)
option(RPU_BUILD_FOR_REV102 "Build for RPU hardware revision 102 (MEGA)" OFF)
option(ENABLE_DEBUG_MESSAGES "Enable debug messages over serial" OFF)

# Determine board type based on revision
if(RPU_BUILD_FOR_REV1 OR RPU_BUILD_FOR_REV2)
    set(ARDUINO_BOARD "nanoatmega328")
    set(RPU_HARDWARE_REV 1)
else()
    set(ARDUINO_BOARD "megaatmega2560")
    if(RPU_BUILD_FOR_REV3)
        set(RPU_HARDWARE_REV 3)
    elseif(RPU_BUILD_FOR_REV102)
        set(RPU_HARDWARE_REV 102)
    else()
        set(RPU_HARDWARE_REV 4)
    endif()
endif()

# Arduino core library
find_package(Arduino REQUIRED)

# Main executable target
set(SOURCES
    src/main.cpp
    src/AudioHandler.cpp
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
    RPU_MPU_ARCHITECTURE=1
    RPU_MPU_BUILD_FOR_6800=1
    RPU_OS_USE_DIP_SWITCHES
    RPU_OS_USE_SB100
)

if(RPU_BUILD_FOR_REV102 AND ENABLE_DEBUG_MESSAGES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        DEBUG_MESSAGES
    )
endif()

# Add RPU library
add_subdirectory(lib/RPU)
target_link_libraries(${PROJECT_NAME} PRIVATE RPU)

# Add WavTrigger library
add_subdirectory(lib/WavTrigger)
target_link_libraries(${PROJECT_NAME} PRIVATE WavTrigger)

# Arduino core and standard libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Arduino::Core
    Arduino::EEPROM
)

# For MEGA 2560, link Serial1 support
if(NOT (RPU_BUILD_FOR_REV1 OR RPU_BUILD_FOR_REV2))
    target_link_libraries(${PROJECT_NAME} PRIVATE Arduino::HardwareSerial)
endif()

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
