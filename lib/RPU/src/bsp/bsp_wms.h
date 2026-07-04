//
// Created by Douglas Bercot on 6/28/26.
//

#ifndef TRIDENT2020RPU_BSP_WMS_H
#define TRIDENT2020RPU_BSP_WMS_H

#include <stdint.h>

constexpr uint16_t PIA_DISPLAY_PORT_A = 0x2800;
constexpr uint16_t PIA_DISPLAY_CONTROL_A = 0x2801;
constexpr uint16_t PIA_DISPLAY_PORT_B = 0x2802;
constexpr uint16_t PIA_DISPLAY_CONTROL_B = 0x2803;
constexpr uint16_t PIA_SWITCH_PORT_A = 0x3000;
constexpr uint16_t PIA_SWITCH_CONTROL_A = 0x3001;
constexpr uint16_t PIA_SWITCH_PORT_B = 0x3002;
constexpr uint16_t PIA_SWITCH_CONTROL_B = 0x3003;
constexpr uint16_t PIA_LAMPS_PORT_A = 0x2400;
constexpr uint16_t PIA_LAMPS_CONTROL_A = 0x2401;
constexpr uint16_t PIA_LAMPS_PORT_B = 0x2402;
constexpr uint16_t PIA_LAMPS_CONTROL_B = 0x2403;
constexpr uint16_t PIA_SOLENOID_PORT_A = 0x2200;
constexpr uint16_t PIA_SOLENOID_CONTROL_A = 0x2201;
constexpr uint16_t PIA_SOLENOID_PORT_B = 0x2202;
constexpr uint16_t PIA_SOLENOID_CONTROL_B = 0x2203;

#if (RPU_MPU_ARCHITECTURE==13)
constexpr uint16_t PIA_SOUND_COMMA_PORT_A = 0x2100;
constexpr uint16_t PIA_SOUND_COMMA_CONTROL_A = 0x2101;
constexpr uint16_t PIA_SOUND_COMMA_PORT_B = 0x2102;
constexpr uint16_t PIA_SOUND_COMMA_CONTROL_B = 0x2103;
#endif

#if (RPU_MPU_ARCHITECTURE==15)
constexpr uint16_t PIA_SOUND_11_PORT_A = 0x2100;
constexpr uint16_t PIA_SOUND_11_CONTROL_A = 0x2101;
constexpr uint16_t PIA_SOLENOID_11_PORT_B = 0x2102;
constexpr uint16_t PIA_SOLENOID_11_CONTROL_B = 0x2103;
constexpr uint16_t PIA_ALPHA_DISPLAY_PORT_A = 0x2C00;
constexpr uint16_t PIA_ALPHA_DISPLAY_CONTROL_A = 0x2C01;
constexpr uint16_t PIA_ALPHA_DISPLAY_PORT_B = 0x2C02;
constexpr uint16_t PIA_ALPHA_DISPLAY_CONTROL_B = 0x2C03;
constexpr uint16_t PIA_NUM_DISPLAY_PORT_A = 0x3400;
constexpr uint16_t PIA_NUM_DISPLAY_CONTROL_A = 0x3401;
constexpr uint16_t PIA_WIDGET_PORT_B = 0x3402;
constexpr uint16_t PIA_WIDGET_CONTROL_B = 0x3403;
#endif


#endif // TRIDENT2020RPU_BSP_WMS_H