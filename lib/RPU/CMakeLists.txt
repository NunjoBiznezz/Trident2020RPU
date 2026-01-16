cmake_minimum_required(VERSION 3.20)

project(RPU)

# Feature options (set by parent or can be configured directly)
option(RPU_ENABLE_DIP_SWITCHES "Enable DIP switch reading" ON)
option(RPU_ENABLE_SB100 "Enable SB-100 sound card support" ON)
option(RPU_ENABLE_SB300 "Enable SB-300 sound card support" ON)
option(RPU_ENABLE_WAV_TRIGGER "Enable WavTrigger support" ON)

# RPU library sources
set(RPU_SOURCES
    RPU.cpp
)

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

# Link Arduino core
target_link_libraries(${PROJECT_NAME} PUBLIC
    Arduino::Core
    Arduino::EEPROM
)

# Export include directories for parent project
set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${RPU_HEADERS}"
)
