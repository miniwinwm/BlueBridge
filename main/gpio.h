/*

MIT License

Copyright (c) John Blaiklock 2022 BlueThing

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef GPIO_H
#define GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/***************
*** INCLUDES ***
***************/

#include <stdbool.h>

/**************
*** DEFINES ***
**************/

#define GPIO_TEST_DATA					GPIO_NUM_33			///< GPIO pin attached to jumper to enable test data (jumper on = test data enabled, jumper off = test data disabled)
#define GPIO_GPS_SELECT					GPIO_NUM_32			///< GPIO pin attached to jumper for GPS source selection (jumper on = NMEA2000, jumper off = NMEA0183)

/************
*** TYPES ***
************/

/*************************
*** EXTERNAL VARIABLES ***
*************************/

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Initialize the GPIO driver. Call once before using other functions
 *
 * @note Currently not used 
 */
void gpio_init(void);

/**
 * Get test data enabled state
 *
 * @return If test data enabled, true else false
 * @note Currently not used
 */
bool gpio_get_test_data_enabled(void);

/**
 * Get GPS source 
 *
 * @return For NMEA2000 false, for NMEA0183 true
 * @note Currently not used
 */
bool gpio_get_gps_data_source(void);

#ifdef __cplusplus
}
#endif

#endif