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

#ifndef UTIL_H
#define UTIL_H

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

/*************************
*** EXTERNAL VARIABLES ***
*************************/

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Convert a hex number in upper or lower case to an int. No leading 0x.
 *
 * @param s The string containing the hex number 
 * @return The converted number
 */
uint32_t util_htoi(const char *s);

/**
 * Replace every instance of a character in a string with another 
 *
 * @param s The string to replace characters in
 * @param orig The character to replace
 * @param rep The character to replace with
 * @return Pointer to the modified string
 */
void util_replace_char(char *s, char orig, char rep);

/**
 * Replace each lower case character with its upper case equivalent
 *
 * @param s String contaiining the characters to check and replace 
 * @return The inpur string or NULL if the input string is NULL
 */
char *util_capitalize_string(char *s);

/**
 * Convert a seconds value to a text representation in format xxhyymzzs
 *
 * @param seconds The input time to convert in seconds 
 * @return The converted text value
 * @note This function is not thread safer
 */
char *util_seconds_to_hms(uint32_t seconds);

/**
 * Parse a text hour/minute/second value in xxhyymzzs format to an integer of the total number of seconds
 *
 * @param hms String containing the value to convert. Must contain at least a number and an h, m or s. The letters h, m and s must be in that order.
 * @param result Pointer to value to hold result
 * @return If successful true else false
 */
bool util_hms_to_seconds(const char *hms, uint32_t *result);

/**
 * Simple hash function of ASCII string using the DJB2 algorithm
 *
 * @param str The string to hash
 * @return The hash value
 */
uint32_t util_hash_djb2(const char *str);

/**
 * Perform a safe equivalent of strcat
 * 
 * @param dest Buffer to hold the concatenated string
 * @param size Size in bytes of dest
 * @param src The source string to append to what already exists in dest
 * @return true if string concatenated successfully
 */
bool util_safe_strcat(char *dest, size_t size, const char *src);

/**
 * Perform a safe equivalent of strcpy.
 *
 * @param dest Pointer to destination string
 * @param size The size of the dest buffer
 * @param src Pointer to source string to copy
 */
bool util_safe_strcpy(char *dest, size_t size, const char *src);

#ifdef __cplusplus
}
#endif

#endif