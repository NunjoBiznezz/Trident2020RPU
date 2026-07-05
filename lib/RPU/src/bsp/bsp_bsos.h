//
// Created by Douglas Bercot on 6/28/26.
//

#ifndef TRIDENT2020RPU_BSP_BSOS_H
#define TRIDENT2020RPU_BSP_BSOS_H

#include <stdint.h>

#if RPU_OS_HARDWARE_REV_IS(1)
constexpr uint16_t ADDRESS_U10_A = 0x14;
constexpr uint16_t ADDRESS_U10_A_CONTROL = 0x15;
constexpr uint16_t ADDRESS_U10_B = 0x16;
constexpr uint16_t ADDRESS_U10_B_CONTROL = 0x17;
constexpr uint16_t ADDRESS_U11_A = 0x18;
constexpr uint16_t ADDRESS_U11_A_CONTROL = 0x19;
constexpr uint16_t ADDRESS_U11_B = 0x1A;
constexpr uint16_t ADDRESS_U11_B_CONTROL = 0x1B;
constexpr uint16_t ADDRESS_SB100 = 0x10;

#elif RPU_OS_HARDWARE_REV_IS(2)

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

#elif RPU_OS_HARDWARE_REV_IS(3) || RPU_OS_HARDWARE_REV_IS(4)

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
#endif

// -17, -35, 100, and 200 MPU boards
// Depending on the number of digits, the RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL
// can be adjusted in order to change the refresh rate of the displays.
// The original -17 / MPU-100 boards ran at 320 Hz
// The Alltek runs the displays at 440 Hz (probably so 7-digit displays won't flicker)
// The value below is calculated with this formula:
//       Value = (interval in ms) * (16*10^6) / (1*1024) - 1
//          (must be <65536)
// Choose one of these values (or do whatever)
//  Value         Frequency
//  48            318.8 Hz
//  47            325.5 Hz
//  46            332.4 Hz increments   (I use this for 6-digits displays)
//  45            339.6 Hz
//  40            381 Hz
//  35            434 Hz     (This would probably be good for 7-digit displays)
//  34            446.4 Hz
constexpr uint16_t RPU_OS_SOFTWARE_DISPLAY_INTERRUPT_INTERVAL = 48;


#endif // TRIDENT2020RPU_BSP_BSOS_H