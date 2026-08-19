#include "RPU.h"
#include "RPU_config.h"
#include "RPU_Internal.h"
#include <Arduino.h>

#ifdef RPU_OS_USE_DASH51


void RPU_PlaySoundDash51(uint8_t soundByte) {
   // This device has 32 possible sounds, but they're mapped to
   // 0 - 15 and then 128 - 143 on the original card, with bits b4, b5, and b6 reserved
   // for timing controls.
   // For ease of use, I've mapped the sounds from 0-31

   uint8_t oldSolenoidControlByte, soundLowerNibble, displayWithSoundBit4, oldDisplayByte;

   // mask further zero-crossing interrupts during this
   noInterrupts();

   // Get the current value of U11:PortB - current solenoids
   oldSolenoidControlByte = RPU_DataRead(ADDRESS_U11_B);
   oldDisplayByte = RPU_DataRead(ADDRESS_U11_A);
   soundLowerNibble = (oldSolenoidControlByte & 0xF0) | (soundByte & 0x0F);
   displayWithSoundBit4 = oldDisplayByte;
   if (soundByte & 0x10) {
      displayWithSoundBit4 |= 0x02;
   } else {
      displayWithSoundBit4 &= 0xFD;
   }

   // Put 1s on momentary solenoid lines
   RPU_DataWrite(ADDRESS_U11_B, oldSolenoidControlByte | 0x0F);

   // Put sound latch low
   RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);

   // Let the strobe stay low for a moment
   delayMicroseconds(68);

   // put bit 4 on Display Enable 7
   RPU_DataWrite(ADDRESS_U11_A, displayWithSoundBit4);

   // Put sound latch high
   RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x3C);

   // put the new uint8_t on U11:PortB (the lower nibble is currently loaded)
   RPU_DataWrite(ADDRESS_U11_B, soundLowerNibble);

   // wait 180 microseconds
   delayMicroseconds(180);

   // Restore the original solenoid uint8_t
   RPU_DataWrite(ADDRESS_U11_B, oldSolenoidControlByte);

   // Restore the original display uint8_t
   RPU_DataWrite(ADDRESS_U11_A, oldDisplayByte);

   // Put sound latch low
   RPU_DataWrite(ADDRESS_U11_B_CONTROL, 0x34);

   interrupts();
}

#endif