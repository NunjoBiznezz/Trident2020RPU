#include "RPU.h"

/******************************************************
 *   Sound Handling Functions
 */

#ifdef RPU_OS_USE_S_AND_T

#include "RPU_Internal.h"
#include <Arduino.h>

void RPU_PlaySoundSAndT(uint8_t soundByte) {
   uint8_t oldSolenoidControlByte, soundLowerNibble, soundUpperNibble;

   // mask further zero-crossing interrupts during this
   noInterrupts();

   // Get the current value of U11:PortB - current solenoids
   oldSolenoidControlByte = RPU_DataRead(ADDRESS_U11_B);
   soundLowerNibble = (oldSolenoidControlByte & 0xF0) | (soundByte & 0x0F);
   soundUpperNibble = (oldSolenoidControlByte & 0xF0) | (soundByte / 16);

   // Put 1s on momentary solenoid lines
   RPU_DataWrite(ADDRESS_U11_B, oldSolenoidControlByte | 0x0F);

   // Put sound latch low
   RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);

   // Let the strobe stay low for a moment
   delayMicroseconds(32);

   // Put sound latch high
   RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x3C);

   // put the new uint8_t on U11:PortB (the lower nibble is currently loaded)
   RPU_DataWrite(ADDRESS_U11_B, soundLowerNibble);

   // wait 138 microseconds
   delayMicroseconds(138);

   // put the new uint8_t on U11:PortB (the uppper nibble is currently loaded)
   RPU_DataWrite(ADDRESS_U11_B, soundUpperNibble);

   // wait 76 microseconds
   delayMicroseconds(145);

   // Restore the original solenoid uint8_t
   RPU_DataWrite(ADDRESS_U11_B, oldSolenoidControlByte);

   // Put sound latch low
   RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);

   interrupts();
}
#endif
