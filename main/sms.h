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

#ifndef SMS_H
#define SMS_H

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

#define SMS_MAX_PHONE_NUMBER_LENGTH		24UL		///< Maximum number of characters in a phone number including international part

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
 * Initialize the SMS library. Call once before using other functions.
 */
void sms_init(void);

/**
 * Check if there is a waiting SMS message and get its id if there is.
 *
 * @param sms_id Pointer to where the id of a waiting message will be saved.
 * @return true if there is a waiting message or false if not or sms_id is NULL.
 * @note If there is no waiting message then sms_id will not be changed.
 */
bool sms_check_for_new(uint32_t *sms_id);

/**
 * Retrieve a received SMS message of known id
 *
 * @param sms_id The id of the message as previously obtained from sms_check_for_new().
 * @param phone_number Pointer to buffer that will contain the sender's phone number of a received message.
 * @param phone_number_buffer_length The length in bytes of phone_number
 * @param message_text Pointer to buffer that will contained the received message text
 * @param message_text_buffer_length The length in bytes of message_text
 */
bool sms_receive(uint32_t sms_id, char *phone_number, size_t phone_number_buffer_length, char *message_text, size_t message_text_buffer_length);

/**
 * Send a SMS message
 *
 * @param message_text The text of the message to send. Only up to 160 characters will be sent.
 * @param phone_number The phone number of the recipient.
 */
bool sms_send(const char *message_text, const char *phone_number);

#ifdef __cplusplus
}
#endif

#endif