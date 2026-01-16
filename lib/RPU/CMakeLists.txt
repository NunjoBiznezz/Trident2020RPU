cmake_minimum_required(VERSION 3.20)

project(RPU)

# Verify required build configuration variables
if(NOT DEFINED RPU_MPU_ARCHITECTURE)
    message(FATAL_ERROR "RPU_MPU_ARCHITECTURE not defined. Please set it in parent CMakeLists.txt")
endif()

if(NOT DEFINED RPU_OS_HARDWARE_REV)
    message(FATAL_ERROR "RPU_OS_HARDWARE_REV not defined. Please set it in parent CMakeLists.txt")
endif()

if(NOT DEFINED RPU_MPU_BUILD_FOR_6800)
    message(FATAL_ERROR "RPU_MPU_BUILD_FOR_6800 not defined. Please set it in parent CMakeLists.txt")
endif()

# Feature options (set by parent or can be configured directly)
option(RPU_ENABLE_DIP_SWITCHES "Enable DIP switch reading" ON)
option(RPU_ENABLE_SB100 "Enable SB-100 sound card support" ON)
option(RPU_ENABLE_SB300 "Enable SB-300 sound card support" ON)
option(RPU_ENABLE_WAV_TRIGGER "Enable WavTrigger support" ON)
option(RPU_ENABLE_7_DIGIT_DISPLAYS "Enable 7-digit displays" OFF)
option(RPU_ENABLE_6_DIGIT_CREDIT_WITH_7_DIGIT "Use 6-digit credit display with 7-digit displays" OFF)
option(RPU_ENABLE_AUX_LAMPS "Enable auxiliary lamps" OFF)

# Validate architecture-specific options
if(RPU_ENABLE_6_DIGIT_CREDIT_WITH_7_DIGIT AND NOT RPU_MPU_ARCHITECTURE EQUAL 1)
    message(FATAL_ERROR "RPU_ENABLE_6_DIGIT_CREDIT_WITH_7_DIGIT is only valid for RPU_MPU_ARCHITECTURE=1 (Bally/Stern -17/-35/100/200). Current architecture: ${RPU_MPU_ARCHITECTURE}")
endif()

if(RPU_ENABLE_AUX_LAMPS AND NOT RPU_MPU_ARCHITECTURE EQUAL 1)
    message(FATAL_ERROR "RPU_ENABLE_AUX_LAMPS is only valid for RPU_MPU_ARCHITECTURE=1 (Bally/Stern -17/-35/100/200). Current architecture: ${RPU_MPU_ARCHITECTURE}")
endif()

# RPU library sources
set(RPU_SOURCES
    RPU.cpp
)
required architecture definitions (passed from parent)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    RPU_MPU_ARCHITECTURE=${RPU_MPU_ARCHITECTURE}
    RPU_OS_HARDWARE_REV=${RPU_OS_HARDWARE_REV}
    RPU_MPU_BUILD_FOR_6800=${RPU_MPU_BUILD_FOR_6800}
)

# Configure 
set(RPU_HEADERS
    RPU.h
    RPU_config.h
    RPU_Addresses.h
)

add_library(${PROJECT_NAME} STATIC ${RPU_SOURCES} ${RPU_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Configure feature flags as compile definitions
if(RPU_ENABLE_DIP_SWITCHES)
    target_compile_definitions(${PROJECT_NAME} PUBLIC RPU_OS_USE_DIP_SWITCHES)
endif()

if(RPU_ENABLE_SB100)
    target_compile_definitions(${PROJECT_NAME} PUBLIC RPU_OS_USE_SB100)
endif()

if(RPU_ENABLE_SB300)
    target_compile_definitions(${PROJECT_NAME} PUBLIC RPU_OS_USE_SB300)
endif()

if(RPU_ENABLE_WAV_TRIGGER)
    target_compile_definitions(${PROJECT_NAME} PUBLIC RPU_OS_USE_WAV_TRIGGER_1p3)
    # Conditionally link WavTrigger library if it's available
    if(TARGET WavTrigger)
        target_link_libraries(${PROJECT_NAME} PUBLIC WavTrigger)
        message(STATUS "RPU: WavTrigger support enabled")
    endif()
endif()

if(RPU_ENABLE_7_DIGIT_DISPLAYS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC RPU_OS_USE_7_DIGIT_DISPLAYS)
endif()

if(RPU_ENABLE_6_DIGIT_CREDIT_WITH_7_DIGIT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS)
endif()

if(RPU_ENABLE_AUX_LAMPS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC RPU_OS_USE_AUX_LAMPS)
endif()

# Link Arduino core
target_link_libraries(${PROJECT_NAME} PUBLIC
    Arduino::Core
    Arduino::EEPROM
)

# Export include directories for parent project
set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${RPU_HEADERS}"
)
