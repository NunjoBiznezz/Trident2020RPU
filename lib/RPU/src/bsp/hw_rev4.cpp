#if (RPU_OS_HARDWARE_REV == 4)

#if !defined(__AVR_ATmega2560__)
#error "RPU_OS_HARDWARE_REV 4 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#include "RPU.h"
#include "RPU_Internal.h"
#include <Arduino.h>

// Rev 4 pin map (MEGA 2560 Pro larger, J5):
//   D3=R/W  D5=BUFFER_DISABLE  D22-D29=DATA  D38=SWITCH  D39=PHI2  D40=VMA
//   D41=HALT  D42=RESET  D44=DIAGNOSTIC  A0-A15=ADDRESS

static constexpr uint8_t RPU_VMA_PIN = 40;
static constexpr uint8_t RPU_RW_PIN = 3;
static constexpr uint8_t RPU_PHI2_PIN = 39;
static constexpr uint8_t RPU_SWITCH_PIN = 38;
static constexpr uint8_t RPU_BUFFER_DISABLE = 5;
static constexpr uint8_t RPU_HALT_PIN = 41;
static constexpr uint8_t RPU_RESET_PIN = 42;
static constexpr uint8_t RPU_DIAGNOSTIC_PIN = 44;

#if !defined(RPU_MPU_BUILD_FOR_6800) || (RPU_MPU_BUILD_FOR_6800 == 1)
static constexpr bool UsesM6800Processor = true;
#else
static constexpr bool UsesM6800Processor = false;
#endif

static void RPU_SetAddressPinsDirection(uint8_t pinsOutput) {
   for (int count = 0; count < 16; count++) {
      pinMode(A0 + count, pinsOutput);
   }
}

static void RPU_SetDataPinsDirection(uint8_t pinsOutput) {
   for (int count = 0; count < 8; count++) {
      pinMode(22 + count, pinsOutput);
   }
}

void RPU_DataWrite(int address, uint8_t data) {
   DDRA = 0xFF;
   PORTE = (PORTE & 0xDF); // R/W LOW

   PORTA = data;
   PORTF = (uint8_t)(address & 0x00FF);
   PORTK = (uint8_t)(address / 256);

   if (UsesM6800Processor) {
      while ((PING & 0x04)) // wait for falling clock edge
         ;
   } else {
      PORTG &= ~0x04; // drive clock low
   }

   PORTG = PORTG | 0x02; // VMA ON

   if (UsesM6800Processor) {
      while (!(PING & 0x04))
         ;
      while ((PING & 0x04))
         ;
      while (!(PING & 0x04))
         ;
   } else {
      PORTG |= 0x04;  // clock high
      PORTG &= ~0x04; // clock low
      PORTG |= 0x04;  // clock high
   }

   PORTG = PORTG & 0xFD; // VMA OFF
   PORTF = 0x00;
   PORTK = 0x00;
   PORTE = (PORTE | 0x20); // R/W HIGH
   DDRA = 0x00;
}

uint8_t RPU_DataRead(int address) {
   DDRA = 0x00;
   DDRE = DDRE | 0x20;
   PORTE = (PORTE | 0x20); // R/W HIGH

   PORTF = (uint8_t)(address & 0x00FF);
   PORTK = (uint8_t)(address / 256);

   if (UsesM6800Processor) {
      while ((PING & 0x04)) // wait for falling clock edge
         ;
   } else {
      PORTG &= ~0x04; // drive clock low
   }

   PORTG = PORTG | 0x02; // VMA ON

   if (UsesM6800Processor) {
      while (!(PING & 0x04))
         ;
      while ((PING & 0x04))
         ;
      while (!(PING & 0x04))
         ;
   } else {
      PORTG |= 0x04;  // clock high
      PORTG &= ~0x04; // clock low
      PORTG |= 0x04;  // clock high
   }

   uint8_t inputData = PINA;

   PORTG = PORTG & 0xFD; // VMA OFF
   PORTE = (PORTE & 0xDF); // R/W LOW
   PORTF = 0x00;
   PORTK = 0x00;

   return inputData;
}

void RPU_HW_SetupPorts(uint16_t &retVal) {
   pinMode(RPU_DIAGNOSTIC_PIN, INPUT);
   if (digitalRead(RPU_DIAGNOSTIC_PIN) == 1) {
      retVal |= RPU_RET_DIAGNOSTIC_REQUESTED;
   }
}

bool RPU_HW_EarlyInit(uint16_t initOptions, uint8_t creditResetSwitch, uint16_t &retVal) {
   pinMode(RPU_BUFFER_DISABLE, OUTPUT);
   digitalWrite(RPU_BUFFER_DISABLE, 1); // tri-state 680X buffers

   pinMode(RPU_HALT_PIN, OUTPUT);
   digitalWrite(RPU_HALT_PIN, 0); // assert /HALT
   pinMode(RPU_RESET_PIN, OUTPUT);
   digitalWrite(RPU_RESET_PIN, 0); // assert /RESET

   pinMode(RPU_VMA_PIN, OUTPUT);
   pinMode(RPU_RW_PIN, OUTPUT);
   RPU_SetAddressPinsDirection(OUTPUT);

   if (UsesM6800Processor) {
      pinMode(RPU_PHI2_PIN, INPUT);
   } else {
      pinMode(RPU_PHI2_PIN, OUTPUT);
   }

   delay(1000);

   pinMode(RPU_SWITCH_PIN, INPUT);
   bool switchStateClosed = digitalRead(RPU_SWITCH_PIN) != 0;
   if (switchStateClosed) {
      retVal |= RPU_RET_SELECTOR_SWITCH_ON;
   }

   bool creditResetButtonHit = false;
   if (creditResetSwitch != 0xFF &&
       (initOptions & (RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET))) {
      creditResetButtonHit = CheckCreditResetSwitchArch1(creditResetSwitch);
      if (creditResetButtonHit) {
         retVal |= RPU_RET_CREDIT_RESET_BUTTON_HIT;
      }
   }

   bool bootToOriginal = (initOptions & RPU_CMD_BOOT_ORIGINAL) ||
                         (switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_SWITCH_CLOSED)) ||
                         (!switchStateClosed && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED)) ||
                         (creditResetButtonHit && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET)) ||
                         (!creditResetButtonHit && (initOptions & RPU_CMD_BOOT_ORIGINAL_IF_NOT_CREDIT_RESET));

   if (bootToOriginal) {
      digitalWrite(RPU_BUFFER_DISABLE, 0); // enable tri-state buffers for 680X
      pinMode(RPU_PHI2_PIN, INPUT);
      pinMode(RPU_VMA_PIN, INPUT);
      pinMode(RPU_RW_PIN, INPUT);
      RPU_SetDataPinsDirection(INPUT);
      RPU_SetAddressPinsDirection(INPUT);
      digitalWrite(RPU_HALT_PIN, 1);  // release /HALT
      digitalWrite(RPU_RESET_PIN, 1); // release /RESET
      retVal |= RPU_RET_ORIGINAL_CODE_REQUESTED;
      if (!(initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN)) {
         while (1)
            ;
      }
      return true;
   }

   return false;
}

#endif // RPU_OS_HARDWARE_REV 4
