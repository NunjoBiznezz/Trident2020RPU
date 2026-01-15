cmake_minimum_required(VERSION 3.20)

project(RPU)

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

# Link Arduino core
target_link_libraries(${PROJECT_NAME} PUBLIC
    Arduino::Core
    Arduino::EEPROM
)

# Export include directories for parent project
set_target_properties(${PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${RPU_HEADERS}"
)
