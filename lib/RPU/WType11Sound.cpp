#include "RPU_config.h"
#include "RPU.h"

#ifdef RPU_OS_USE_WTYPE_11_SOUND
void RPU_PlayW11Sound(uint8_t soundNum) {
   RPU_DataWrite(PIA_SOUND_11_PORT_A, soundNum);
   // Strobe CA2
   RPU_DataWrite(PIA_SOUND_11_CONTROL_A, 0x34);
   RPU_DataWrite(PIA_SOUND_11_CONTROL_A, 0x3C);
}

void RPU_PlayW11Music(uint8_t songNum) {
   RPU_DataWrite(PIA_WIDGET_PORT_B, songNum);
   // Strobe CA2
   RPU_DataWrite(PIA_WIDGET_CONTROL_B, 0x34);
   RPU_DataWrite(PIA_WIDGET_CONTROL_B, 0x3C);
}
#endif

