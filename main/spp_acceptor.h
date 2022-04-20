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

This work is based on code in file BluetoothSerial.cpp distributed as part of 
the ESP32 driver layer of the Arduino library for the ESP32 platform. The 
license for the source code is given below:

// Copyright 2018 Evandro Luis Copercini
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License. 

The above source code has been modified to enable it to build directly 
with the ESP32-IDF framework.

*/

#ifndef SPP_ACCEPTOR_H
#define SPP_ACCEPTOR_H

#ifdef __cplusplus
 extern "C" {
#endif

/***************
*** INCLUDES ***
***************/
 
#include <stdlib.h>

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
 * Initialize the serial port profile Bluetooth driver
 */
void spp_init(void);

/**
 * Write data using the Bluetooth serial port profile driver
 * 
 * @param buffer The data to write
 * @param size The length of data in buffer
 * @return How many bytes were written
 */
size_t spp_write(const uint8_t *buffer, size_t size);

/**
 * Read a single byte using the Bluetooth serial port profile driver
 * 
 * @return The byte read or -1 if not possible
 */
int spp_read(void);

/**
 * Get the length of data to read from the Bluetooth serial port profile connection
 *
 * @return Bytes waiting to be read
 */
size_t spp_bytes_received_size(void);

#ifdef __cplusplus
}
#endif

#endif
