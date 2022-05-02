/*

MIT License

Copyright (c) John Blaiklock 2022 BlueBridge

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

#ifndef SERIAL_H
#define SERIAL_H

#ifdef __cplusplus
 extern "C" {
#endif

/***************
*** INCLUDES ***
***************/

#include <stdint.h>
#include <stddef.h>

/**************
*** DEFINES ***
**************/

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
 * Initialize the serial ports, call this once at startuo before using any other functions
 *
 * @param baud_rate_1 Baud rate to use to initialize serial port 1
 * @param baud_rate_2 Baud rate to use to initialize serial port 2
 */
void serial_init(uint32_t baud_rate_1, uint32_t baud_rate_2);

/**
 * Read data from serial port 1
 *
 * @param buffer_length The length of the supplied buffer
 * @param data The buffer to read data into
 * @return How many bytes were read
 */
size_t serial_1_read_data(size_t buffer_length, uint8_t *data);

/**
 * Write data to serial port 1
 *
 * @param length The length of data
 * @param data The data to write
 * @return How many bytes were written
 */
size_t serial_1_send_data(size_t length, const uint8_t *data);

/**
 * Read data from serial port 2
 *
 * @param buffer_length The length of the supplied buffer
 * @param data The buffer to read data into
 * @return How many bytes were read
 */
size_t serial_2_read_data(size_t buffer_length, uint8_t *data);

/**
 * Write data to serial port 2
 *
 * @param length The length of data
 * @param data The data to write
 * @return How many bytes were written
 */
size_t serial_2_send_data(size_t length, const uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
