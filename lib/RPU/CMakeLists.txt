cmake_minimum_required(VERSION 3.20)

project(RPU)

set(RPU_SOURCES
        src/RPU.cpp
        src/bsp/bsos_rev1_2.cpp
        src/bsp/bsos_rev3.cpp
        src/bsp/bsos_rev4.cpp
        src/bsp/bsos_rev100.cpp
        src/bsp/bsos_rev101_102.cpp
)

# Conditionally include native sound card implementations
if(RPU_ENABLE_SB100)
    list(APPEND RPU_SOURCES src/SB100Sound.cpp)
endif()
if(RPU_ENABLE_S_AND_T)
    list(APPEND RPU_SOURCES src/SquawkAndTalk.cpp)
endif()
if(RPU_ENABLE_DASH51)
    list(APPEND RPU_SOURCES src/Dash51Sound.cpp)
endif()

add_library(RPU STATIC ${RPU_SOURCES})

target_include_directories(RPU PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Hardware and architecture defines — PUBLIC so they propagate to any consumer.
# The game target does not need to re-declare these.
target_compile_definitions(RPU PUBLIC
    RPU_OS_HARDWARE_REV=${RPU_OS_HARDWARE_REV}
    RPU_MPU_BUILD_FOR_6800=${RPU_MPU_BUILD_FOR_6800}
)

if(RPU_ENABLE_DIP_SWITCHES)
    target_compile_definitions(RPU PUBLIC RPU_OS_USE_DIP_SWITCHES)
endif()
if(RPU_ENABLE_SB100)
    target_compile_definitions(RPU PUBLIC RPU_OS_USE_SB100)
endif()
if(RPU_ENABLE_SB300)
    target_compile_definitions(RPU PUBLIC RPU_OS_USE_SB300)
endif()
if(RPU_ENABLE_S_AND_T)
    target_compile_definitions(RPU PUBLIC RPU_OS_USE_S_AND_T)
endif()
if(RPU_ENABLE_DASH51)
    target_compile_definitions(RPU PUBLIC RPU_OS_USE_DASH51)
endif()
if(RPU_ENABLE_WAV_TRIGGER)
    target_compile_definitions(RPU PUBLIC RPU_OS_USE_WAV_TRIGGER)
endif()
if(RPU_ENABLE_7_DIGIT_DISPLAYS)
    target_compile_definitions(RPU PUBLIC RPU_OS_USE_7_DIGIT_DISPLAYS)
endif()
if(RPU_ENABLE_6_DIGIT_CREDIT_WITH_7_DIGIT)
    target_compile_definitions(RPU PUBLIC RPU_OS_USE_6_DIGIT_CREDIT_DISPLAY_WITH_7_DIGIT_DISPLAYS)
endif()
if(RPU_ENABLE_AUX_LAMPS)
    target_compile_definitions(RPU PUBLIC RPU_OS_USE_AUX_LAMPS)
endif()

target_link_libraries(RPU PUBLIC
    Arduino::Core
    Arduino::EEPROM
    Arduino::HardwareSerial
)
