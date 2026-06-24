#pragma once

// Forward declarations of internal RPU bus functions defined in RPU.cpp
void RPU_DataWrite(int address, uint8_t data);
uint8_t RPU_DataRead(int address);

constexpr unsigned RPU_NUM_SOLENOIDS = 15;
constexpr unsigned DEFAULT_SOLENOID_STATE = 0x9F;
constexpr uint8_t SOLENOID_STACK_EMPTY = 0xFF;

// Internal solenoid stack operations (also called from RPU_Switches.cpp)
void PushToFrontOfSolenoidStack(uint8_t solenoidNumber, uint8_t numPushes);
uint8_t PullFirstFromSolenoidStack();

void RPU_ResetSolenoidState();
void RPU_InitSolenoidDefault();
void RPU_ServiceSolenoids();
