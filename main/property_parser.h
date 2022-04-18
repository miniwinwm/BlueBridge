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

#ifndef PROPERTY_PARSER_H
#define PROPERTY_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

/***************
*** INCLUDES ***
***************/

#include <stdint.h>
#include <stdbool.h>

/**************
*** DEFINES ***
**************/

/************
*** TYPES ***
************/

/**
 * Callback function called by parser when a key/value pair or command is found.
 *
 * @param key String containg the key or command
 * @param value String containing the value or empty string for a command
 * @note All \n and = values are removed
 */
typedef bool (*parser_callback_t)(char *key, char *value);

/*************************
*** EXTERNAL VARIABLES ***
*************************/

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Parse key/value pairs in format key=value\n. There can be multiple key/value pairs. Also parses commands in
 * format command\n. In this case the returned value is an empty string.
 *
 * @param str The string to parse. The last item does not need to end in a \n.
 * @param parser_callback A callback function that is called when a key/value pair or a command is found
 * @return The number of key/value and commands found.
 */
uint16_t property_parse(char *str, parser_callback_t parser_callback);

#ifdef __cplusplus
}
#endif

#endif