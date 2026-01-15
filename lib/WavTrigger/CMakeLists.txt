cmake_minimum_required(VERSION 3.20)

project(WavTrigger)

# WavTrigger library sources
set(WAVTRIGGER_SOURCES
    WavTrigger.cpp
)

set(WAVTRIGGER_HEADERS
    WavTrigger.h
)

add_library(${PROJECT_NAME} STATIC ${WAVTRIGGER_SOURCES} ${WAVTRIGGER_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link Arduino core (needs HardwareSerial for UART communication)
target_link_libraries(${PROJECT_NAME} PUBLIC
    Arduino::Core
)

# Export include directories for parent project
set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${WAVTRIGGER_HEADERS}"
)
