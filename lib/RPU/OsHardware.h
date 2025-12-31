#ifndef OS_HARDWARE_H
#define OS_HARDWARE_H

#include "RPU_config.h"
#include <stdint.h>


/******************************************************
 *   Hardware Interface Functions
 *
 *   These functions have conditional compilation for different RPU_OS_HARDWARE_REVs
 *
 *   RPU_OS_HARDWARE_REV 1 - Nano board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *   RPU_OS_HARDWARE_REV 2 - Nano board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *                           adds support for SB300 sound cards
 *   RPU_OS_HARDWARE_REV 3 - MEGA2560 PRO board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *                           adds support for full address space
 *   RPU_OS_HARDWARE_REV 4 - MEGA2560 PRO board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *                           adds support for OLED display, WIFI, and multiple serial ports
 *   RPU_OS_HARDWARE_REV 4 - MEGA2560 PRO board that plugs into J5 (only works on -17, -35, 100, and 200 MPUs)
 *                           adds support for OLED display, WIFI, and multiple serial ports
 *   RPU_OS_HARDWARE_REV 100 - MEGA2560 PRO board that plugs into processor socket (prototype)
 *   RPU_OS_HARDWARE_REV 101 - MEGA2560 PRO board that plugs into processor socket
 *                             adds support for multiple serial ports (limited release)
 *   RPU_OS_HARDWARE_REV 102 - MEGA2560 PRO board that plugs into processor socket (prototype)
 *                             adds support for OLED display, WIFI, autodetection of processor type
 *
 */

#if (RPU_OS_HARDWARE_REV == 1)
#if (RPU_MPU_ARCHITECTURE != 1)
#error "RPU_OS_HARDWARE_REV 1 only works on machines with RPU_MPU_ARCHITECTURE of 1"
#endif
constexpr uint16_t ADDRESS_U10_A = 0x14;
constexpr uint16_t ADDRESS_U10_A_CONTROL = 0x15;
constexpr uint16_t ADDRESS_U10_B = 0x16;
constexpr uint16_t ADDRESS_U10_B_CONTROL = 0x17;
constexpr uint16_t ADDRESS_U11_A = 0x18;
constexpr uint16_t ADDRESS_U11_A_CONTROL = 0x19;
constexpr uint16_t ADDRESS_U11_B = 0x1A;
constexpr uint16_t ADDRESS_U11_B_CONTROL = 0x1B;
constexpr uint16_t ADDRESS_SB100 = 0x10;

#elif (RPU_OS_HARDWARE_REV == 2)
#if (RPU_MPU_ARCHITECTURE != 1)
#error "RPU_OS_HARDWARE_REV 2 only works on machines with RPU_MPU_ARCHITECTURE of 1"
#endif
constexpr uint16_t ADDRESS_U10_A = 0x00;
constexpr uint16_t ADDRESS_U10_A_CONTROL = 0x01;
constexpr uint16_t ADDRESS_U10_B = 0x02;
constexpr uint16_t ADDRESS_U10_B_CONTROL = 0x03;
constexpr uint16_t ADDRESS_U11_A = 0x08;
constexpr uint16_t ADDRESS_U11_A_CONTROL = 0x09;
constexpr uint16_t ADDRESS_U11_B = 0x0A;
constexpr uint16_t ADDRESS_U11_B_CONTROL = 0x0B;
constexpr uint16_t ADDRESS_SB100 = 0x10;
constexpr uint16_t ADDRESS_SB100_CHIMES = 0x18;
constexpr uint16_t ADDRESS_SB300_SQUARE_WAVES = 0x10;
constexpr uint16_t ADDRESS_SB300_ANALOG = 0x18;

#elif (RPU_OS_HARDWARE_REV == 3) || (RPU_OS_HARDWARE_REV == 4)
#if (RPU_MPU_ARCHITECTURE != 1)
#error "RPU_OS_HARDWARE_REV 3 and 4 only work on machines with RPU_MPU_ARCHITECTURE of 1"
#endif
constexpr uint16_t ADDRESS_U10_A = 0x88;
constexpr uint16_t ADDRESS_U10_A_CONTROL = 0x89;
constexpr uint16_t ADDRESS_U10_B = 0x8A;
constexpr uint16_t ADDRESS_U10_B_CONTROL = 0x8B;
constexpr uint16_t ADDRESS_U11_A = 0x90;
constexpr uint16_t ADDRESS_U11_A_CONTROL = 0x91;
constexpr uint16_t ADDRESS_U11_B = 0x92;
constexpr uint16_t ADDRESS_U11_B_CONTROL = 0x93;
constexpr uint16_t ADDRESS_SB100 = 0xA0;
constexpr uint16_t ADDRESS_SB100_CHIMES = 0xC0;
constexpr uint16_t ADDRESS_SB300_SQUARE_WAVES = 0xA0;
constexpr uint16_t ADDRESS_SB300_ANALOG = 0xC0;

#elif (RPU_OS_HARDWARE_REV >= 100)
#if (RPU_MPU_ARCHITECTURE < 10)
constexpr uint16_t ADDRESS_U10_A = 0x88;
constexpr uint16_t ADDRESS_U10_A_CONTROL = 0x89;
constexpr uint16_t ADDRESS_U10_B = 0x8A;
constexpr uint16_t ADDRESS_U10_B_CONTROL = 0x8B;
constexpr uint16_t ADDRESS_U11_A = 0x90;
constexpr uint16_t ADDRESS_U11_A_CONTROL = 0x91;
constexpr uint16_t ADDRESS_U11_B = 0x92;
constexpr uint16_t ADDRESS_U11_B_CONTROL = 0x93;
constexpr uint16_t ADDRESS_SB100 = 0xA0;
constexpr uint16_t ADDRESS_SB100_CHIMES = 0xC0;
constexpr uint16_t ADDRESS_SB300_SQUARE_WAVES = 0xA0;
constexpr uint16_t ADDRESS_SB300_ANALOG = 0xC0;
#else
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
#if (RPU_MPU_ARCHITECTURE == 13)
constexpr uint16_t PIA_SOUND_COMMA_PORT_A = 0x2100;
constexpr uint16_t PIA_SOUND_COMMA_CONTROL_A = 0x2101;
constexpr uint16_t PIA_SOUND_COMMA_PORT_B = 0x2102;
constexpr uint16_t PIA_SOUND_COMMA_CONTROL_B = 0x2103;
#endif
#if (RPU_MPU_ARCHITECTURE == 15)
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
#endif

#endif

#endif // OS_HARDWARE_H
