#if (RPU_OS_HARDWARE_REV == 101) || (RPU_OS_HARDWARE_REV == 102)

#if !defined(__AVR_ATmega2560__)
#error "RPU_OS_HARDWARE_REV >100 requires ATMega2560, check RPU_Config.h and adjust settings"
#endif

#include "RPU.h"
#include "RPU_Internal.h"
#include <Arduino.h>

// Rev 101/102 pin map (MEGA 2560 CPU interposer):
//   D3=R/W  D5=BUFFER_DISABLE  D6=DISABLE_PHI_FROM_CPU  D7=DISABLE_PHI_FROM_MPU
//   D22-D29=DATA  D30-D33=BOARD_SEL  D38=SWITCH  D39=PHI2  D40=VMA
//   D41=HALT  D42=RESET  D43=BA  D44=DIAGNOSTIC  A0-A15=ADDRESS

static constexpr uint8_t RPU_VMA_PIN = 40;
static constexpr uint8_t RPU_RW_PIN = 3;
static constexpr uint8_t RPU_PHI2_PIN = 39;
static constexpr uint8_t RPU_SWITCH_PIN = 38;
static constexpr uint8_t RPU_BUFFER_DISABLE = 5;
static constexpr uint8_t RPU_HALT_PIN = 41;
static constexpr uint8_t RPU_RESET_PIN = 42;
static constexpr uint8_t RPU_BA_PIN = 43;
static constexpr uint8_t RPU_DIAGNOSTIC_PIN = 44;
static constexpr uint8_t RPU_DISABLE_PHI_FROM_MPU = 7;
static constexpr uint8_t RPU_DISABLE_PHI_FROM_CPU = 6;
static constexpr uint8_t RPU_BOARD_SEL_0 = 30;
static constexpr uint8_t RPU_BOARD_SEL_1 = 31;
static constexpr uint8_t RPU_BOARD_SEL_2 = 32;
static constexpr uint8_t RPU_BOARD_SEL_3 = 33;

#if !defined(RPU_MPU_BUILD_FOR_6800) || (RPU_MPU_BUILD_FOR_6800 == 1)
#if (RPU_OS_HARDWARE_REV == 102)
static bool UsesM6800Processor = true; // set at runtime by CheckForMPUClock
#else
static constexpr bool UsesM6800Processor = true;
#endif
#else
static constexpr bool UsesM6800Processor = false;
#endif

static void RPU_SetAddressPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 16; count++) {
      pinMode(A0 + count, pinsOutput ? OUTPUT : INPUT);
   }
}

static void RPU_SetDataPinsDirection(bool pinsOutput) {
   for (int count = 0; count < 8; count++) {
      pinMode(22 + count, pinsOutput ? OUTPUT : INPUT);
   }
}

#if (RPU_OS_HARDWARE_REV == 102)
static bool CheckForMPUClock() {
   pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
   digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 1);
   pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
   digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);
   pinMode(RPU_PHI2_PIN, INPUT_PULLUP);

   unsigned long startTime = millis();
   int sawClockLow = 0;
   int sawClockHigh = 0;
   while (millis() < (startTime + 10)) {
      if (PING & 0x04) {
         sawClockHigh += 1;
      } else {
         sawClockLow += 1;
      }
   }

   if (sawClockLow > 25 && sawClockHigh > 25) {
      return true;
   }

   // MPU clock not seen; check if Arduino-generated clock is present
   digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 0);

   sawClockLow = 0;
   sawClockHigh = 0;
   uint8_t lastState = 0;
   for (int count = 0; count < 1000; count++) {
      if (PING & 0x04 && !lastState) {
         sawClockHigh += 1;
         lastState = 1;
      } else if (lastState) {
         sawClockLow += 1;
         lastState = 0;
      }
   }

   digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);
   return false;
}
#endif // RPU_OS_HARDWARE_REV == 102

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

   const uint8_t inputData = PINA;

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

uint16_t RPU_InitializeBSP(uint16_t initOptions, uint8_t creditResetSwitch) {
   uint16_t retVal = 0;

   pinMode(RPU_BUFFER_DISABLE, OUTPUT);
   digitalWrite(RPU_BUFFER_DISABLE, 1); // tri-state 680X buffers

   pinMode(RPU_HALT_PIN, OUTPUT);
   digitalWrite(RPU_HALT_PIN, 0); // assert /HALT
   pinMode(RPU_RESET_PIN, OUTPUT);
   digitalWrite(RPU_RESET_PIN, 0); // assert /RESET

   pinMode(RPU_VMA_PIN, OUTPUT);
   pinMode(RPU_RW_PIN, OUTPUT);
   RPU_SetAddressPinsDirection(true);

#if (RPU_OS_HARDWARE_REV == 102)
   UsesM6800Processor = CheckForMPUClock();
#endif

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

#if (RPU_OS_HARDWARE_REV == 102)
      if (UsesM6800Processor) {
         pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
         digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 0);
         pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
         digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 1);
         retVal |= RPU_RET_6800_DETECTED;
      } else {
         pinMode(RPU_DISABLE_PHI_FROM_MPU, OUTPUT);
         digitalWrite(RPU_DISABLE_PHI_FROM_MPU, 1);
         pinMode(RPU_DISABLE_PHI_FROM_CPU, OUTPUT);
         digitalWrite(RPU_DISABLE_PHI_FROM_CPU, 0);
         retVal |= RPU_RET_6802_OR_8_DETECTED;
      }
#endif

      RPU_SetDataPinsDirection(false);
      RPU_SetAddressPinsDirection(false);
      digitalWrite(RPU_HALT_PIN, 1);  // release /HALT
      digitalWrite(RPU_RESET_PIN, 1); // release /RESET
      retVal |= RPU_RET_ORIGINAL_CODE_REQUESTED;
      if (!(initOptions & RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN)) {
         while (1)
            ;
      }
      return retVal;
   }

   return retVal;
}

#endif // RPU_OS_HARDWARE_REV 101 or 102
