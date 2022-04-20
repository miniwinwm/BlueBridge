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

/***************
*** INCLUDES ***
***************/

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include "nmea.h"
#include "serial.h"
#include "timer.h"

/**************
*** DEFINES ***
**************/

#define NMEA_MESSAGE_MAP_ENTRIES (sizeof(nmea_message_type_map) / sizeof(nmea_message_type_map_t))		///< Number of entries in the mapping table of message headers to message types

/************
*** TYPES ***
************/

/**
 * Structure to hold information on messages to be transmitted
 */
typedef struct
{
    const transmit_message_details_t *transmit_message_details;		///< Pointer to a transmit message details structure owned by the application
    uint32_t next_transmit_time;                                    ///< Time in milliseconds when the next transmit is scheduled
    uint32_t current_transmit_period_ms;                            ///< Transmit period in ms which may be throttled in out of bandwidth situation
    bool transmit_now;                                              ///< Transmit immediately required flag
} transmit_message_info_t;

/**
 * Structure of a line in the table that maps message headers to message types
 */
typedef struct
{
	const char message_header[4];		///< The message header
	nmea_message_type_t message_type;	///< The message type
} nmea_message_type_map_t;

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

static uint8_t decode(const char *buffer, uint8_t port);
static nmea_error_t encode(const transmit_message_info_t *transmit_message_info, char *output_buffer);
static void adjust_messages_speed(uint8_t port, uint32_t permil_period_adjustment);
static const nmea_receive_message_details_t *get_receive_message_details(uint8_t port, nmea_message_type_t message_type);
static const transmit_message_details_t *get_transmit_message_details(uint8_t port, nmea_message_type_t message_type);
static transmit_message_info_t *get_transmit_message_info(uint16_t details_number);
static const char *my_strtok(const char *s1, const char *delimit);
static nmea_message_type_t get_message_type_from_header(const char *header);
static uint8_t count_commas(const char *text);
static const char *my_ftoa(float number, uint8_t precision, uint8_t padding);
static const char *my_itoa(int32_t number);
static uint32_t my_xtoi(const char *hex_string);
static uint8_t calc_checksum(const char *message);
static const char *create_checksum(const char *message);
static bool verify_checksum(const char *message);
static nmea_error_t send_data(uint8_t port, uint16_t data_size, const uint8_t *data, uint16_t *data_sent);
static uint16_t receive_data(uint8_t port, uint16_t buffer_length, uint8_t *data);
static bool util_safe_strcat(char *dest, size_t size, const char *src);
static bool check_received_message(const char *message_data, uint8_t min_commas, uint8_t max_commas);
static uint8_t nmea_count_set_bits(uint32_t n, uint8_t start_bit, uint8_t length);

/**********************
*** LOCAL VARIABLES ***
**********************/

/**
 * Array of pointers to receive message details structures which are owned by the application. 
 */
static const nmea_receive_message_details_t *receive_message_details[NMEA_MAXIMUM_RECEIVE_MESSAGE_DETAILS];

 /**
  * Array of structures holding information on transmit messages, including their details structure pointer. 
  */
static transmit_message_info_t transmit_messages_infos[NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS];

/**
 * Array of buffers (1 per port) of overflowed data that could not be sent but to be sent on next send/receive cycle. 
 */
static char message_data_to_send_buffer[NMEA_NUMBER_OF_PORTS][NMEA_MAX_MESSAGE_LENGTH + 1];

/**
 * Array of buffers (1 per port) of unprocessed received data that could not be decoded because it contains 
 * only part of a message to be processed on next send/receive cycle. 
 */
static char message_data_to_read_buffer[NMEA_NUMBER_OF_PORTS][NMEA_MAX_MESSAGE_LENGTH + 1];

/***********************
*** GLOBAL VARIABLES ***
***********************/

/****************
*** CONSTANTS ***
****************/

/** 
 * Map of nmea message headers to types - receive message types only
 */
static const nmea_message_type_map_t nmea_message_type_map[] = {
		{"GGA", nmea_message_GGA},
		{"RMC", nmea_message_RMC},
		{"VDM", nmea_message_VDM}};
		
/**********************
*** LOCAL FUNCTIONS ***
**********************/		

/**
 * Top level decoder function that later calls individual message type decoders
 *
 * @param buffer Buffer containing received messager to decode
 * @param port The port the message was receivbed on
 * @return The bytes taken from buffer
 */
static uint8_t decode(const char *buffer, uint8_t port)
{
    uint8_t bytes_used = 0U;
    uint8_t next_message_next_position = 0U;
    char next_byte;
    uint8_t next_byte_position = 0U;
    bool in_message = false;
    char next_message[NMEA_MAX_MESSAGE_LENGTH + 1];
    nmea_message_type_t message_type;
    nmea_receive_message_callback_t receive_message_callback;
    const nmea_receive_message_details_t *receive_message_details;

    while (next_byte_position < NMEA_MAX_MESSAGE_LENGTH)
    {
        next_byte = buffer[next_byte_position];

        if (next_byte == 0)
        {
            break;
        }

        if (!in_message)
        {
            if (next_byte == '$' || next_byte == '!' )
            {
                in_message = true;
                next_message[0] = next_byte;
                next_message_next_position = 1U;
            }
            else
            {
                bytes_used++;
            }
        }
        else
        {
            next_message[next_message_next_position] = next_byte;
            next_message_next_position++;

            if (next_byte == '\n')
            {
                in_message = false;
                next_message[next_message_next_position] = 0;

                bytes_used += (uint8_t)strlen(next_message);

                if (strlen(next_message) >= (size_t)(NMEA_MIN_MESSAGE_LENGTH))
                {
                    if (verify_checksum(next_message))
                    {
                        message_type = get_message_type_from_header(&next_message[3]);

                        receive_message_details = get_receive_message_details(port, message_type);
                        if (receive_message_details != NULL)
                        {
                        	receive_message_callback = receive_message_details->receive_message_callback;

                            if (receive_message_callback)
                            {
                            	receive_message_callback(next_message);
                            }
                        }
                    }
                }
            }
        }

        next_byte_position++;
    }

    if (next_byte_position == NMEA_MAX_MESSAGE_LENGTH && bytes_used == 0U)
    {
        bytes_used = NMEA_MAX_MESSAGE_LENGTH;
    }

    return bytes_used;
}

/**
 * Top level encoder function that later calls individual message type encoders
 *
 * @param transmit_message_info Structure holding details of the message to encode
 * @param output_buffer Buffer to hold encoded message
 * @return The bytes taken from buffer
 */
static nmea_error_t encode(const transmit_message_info_t *transmit_message_info, char *output_buffer)
{
    nmea_get_transmit_data_callback_t transmit_data_callback;
    nmea_encoder_function_t encoder;
    void *message_data;
    nmea_error_t error = nmea_error_param;

    if (transmit_message_info != NULL && transmit_message_info->transmit_message_details != NULL)
    {
    	transmit_data_callback = transmit_message_info->transmit_message_details->get_transmit_data_callback;
        encoder = transmit_message_info->transmit_message_details->encoder;

		if (transmit_data_callback != NULL && encoder != NULL)
		{
			transmit_data_callback();
			message_data = transmit_message_info->transmit_message_details->get_data_structure;

			if (message_data != NULL)
			{
				error = encoder(output_buffer, message_data);

				if (error == nmea_error_none)
				{
					(void)strcat(output_buffer, create_checksum(output_buffer + 1));
					(void)strcat(output_buffer, "\r\n");
				}
			}
		}
    }

    return error;
}

/**
 * Get an enumerated message type from the header string
 *
 * @param header 3 letter string
 * @return The enumerated message type
 */
static nmea_message_type_t get_message_type_from_header(const char *header)
{
	for (uint8_t i = 0U; i < NMEA_MESSAGE_MAP_ENTRIES; i++)
	{
		if (strncmp(header, nmea_message_type_map[i].message_header, (size_t)3) == 0)
		{
			return nmea_message_type_map[i].message_type;
		}
	}

    return nmea_message_min;
}

/**
 * Calculate a checksum on a message.
 *
 * @param pMessage The message to calculate the checksum for. Initial $ must not be present.
 * @return The checksum.
 */
static uint8_t calc_checksum(const char *message)
{
    uint8_t checksum = 0U;

    while (*message)
    {
        checksum ^= *message++;
    }

    return checksum;
}

/**
 * Convert a hex number in upper or lower case to an int. No leading 0x.
 *
 * @param hex_string The string containing the hex number 
 * @return The converted number
 */
static uint32_t my_xtoi(const char *hex_string)
{
	uint32_t i = 0UL;

	while (*hex_string)
	{
		char c = toupper(*hex_string++);
		if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A')))
		{
			break;
		}
		c -= '0';

		if (c > 9)
		{
			c -= 7;
		}

		i = (i << 4) + (uint32_t)c;
	}

	return i;
}

/**
 * Test a NMEA0183 checksum on a message
 *
 * @param pMessage The whole message to test the checksum for
 * @return true if checksum is ok otherwise false
 */
static bool verify_checksum(const char *message)
{
    size_t length;
    uint8_t calculated_checksum = 0U;
    uint32_t read_checksum;
    char *checksum_start;

    length = strlen(message);

    if (length < (size_t)12)
    {
        return false;
    }

    if ((checksum_start = strchr(message, '*')) == NULL)
    {
        return false;
    }

    message++;
    while (*message != '*')
    {
        calculated_checksum ^= *message++;
    }

    read_checksum = my_xtoi(checksum_start + 1);

    return ((uint32_t)calculated_checksum == read_checksum);
}

/**
 * Calculate and create a text checksum for a message
 *
 * @param pMessage The message to calculate the checksum for
 * @return The checksum text
 * @note Not thread safe
 */
static const char *create_checksum(const char *message)
{
    static char checksum_text[4];

    (void)snprintf(checksum_text, sizeof(checksum_text), "*%02X", calc_checksum(message));

    return checksum_text;
}

/**
 * Counts the number of commas in the given string
 *
 * @param   pString  pointer to the start of the string to count its commas
 * @return  Count of commas
 */
static uint8_t count_commas(const char *text)
{
    uint8_t comma_count = 0U;

    while (*text)
    {
        if (*text == ',')
        {
        	comma_count++;
        }
        text++;
    }

    return comma_count;
}

/**
 * Convert a floating point number to a string with specified decimal places
 *
 * @param number The number to convert
 * @param precision The number of decimal places
 * @param padding Number of zeros to pre-pad with, 0 means no pre-padding
 * @return A pointer to a static string containing the result.
 * @note The internal buffer is 24 bytes long and number must fit in this
 * @note Not thread safe
 */
static const char *my_ftoa(float number, uint8_t precision, uint8_t padding)
{
    static char buffer[24];
    char format[9];

    buffer[0] = 0;

    if (precision > 12U || padding > 12U)
    {
        return buffer;
    }

    if (padding > 0U)
    {
        if (precision > 0U)
        {
        	(void)snprintf(format, sizeof(format), "%%0%u.%uf", padding + precision + 1U, precision);
        }
        else
        {
        	(void)snprintf(format, sizeof(format), "%%0%u.%uf", padding + precision, precision);
        }
    }
    else
    {
        (void)snprintf(format, sizeof(format), "%%.%uf", precision);
    }

    (void)snprintf(buffer, sizeof(buffer), format, number);

    return buffer;
}

/**
 * Convert an integer number to a string
 *
 * @param number The number to convert
 * @return A pointer to a static string containing the result
 * @note The internal buffer is 12 bytes long and number must fit in this
 * @note Not thread safe
 */
static const char *my_itoa(int32_t number)
{
    static char buffer[12];

    (void)snprintf(buffer, sizeof(buffer), "%d", (int)number);

    return buffer;
}

/**
 * Private implementation of strtok as the C library version is not thread safe.
 *
 * @param s1 The string to parse.
 * @param delimit Array of delimiter characters.
 * @return The next token or NULL.
 * @note This works the same as the C library strtok and is still not thread safe but will not clash with
 *       uses of strtok in other threads if there are any.
 */
static const char *my_strtok(const char *s1, const char *delimit)
{
    static char *last_token = NULL;
    char *tmp;

    if (s1 == NULL)
    {
        s1 = last_token;
        if (s1 == NULL)
        {
            return NULL;
        }
    }
    else
    {
        s1 += strspn(s1, delimit);
    }

    tmp = strpbrk(s1, delimit);
    if (tmp)
    {
        *tmp = 0;
        last_token = tmp + 1;
    }
    else
    {
    	last_token = NULL;
    }

    return s1;
}

/**
 * Count the number of set bits in a bitfield
 *
 * @param n The bitfield
 * @param start_bit The bit to start counting from
 * @param length The length of the bitfield in n
 * @return The count of set bits found
 */
static uint8_t nmea_count_set_bits(uint32_t n, uint8_t start_bit, uint8_t length)
{
    uint8_t count = 0U;
    uint32_t mask;
    uint8_t bit;

    mask = 1UL;
    for (bit = 0U; bit < start_bit + length; bit++)
    {
        if (bit >= start_bit && (n & mask) > 0UL)
        {
        	count++;
        }
        mask <<= 1;
    }

    return count;
}

/**
 * Get a pointer to a transmit message info structure.
 *
 * @param details_number The slot to get the transmit info structure from.
 * @return Pointer to the structure of NULL if details_number is out of range.
 */
static transmit_message_info_t *get_transmit_message_info(uint16_t details_number)
{
    if (details_number < NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS)
    {
        return &transmit_messages_infos[details_number];
    }

    return NULL;
}

/**
 * Adjust message transmit period in case of not enough bandwidth to sent all required messages or bring period back up if previously slowed down
 *
 * @param port The port the message is transmitted on
 * @param permil_period_adjustment Part per thousand adjustment : < 1000 speeds up, > 1000 slows down
 */
static void adjust_messages_speed(uint8_t port, uint32_t permil_period_adjustment)
{
    uint16_t i;

    for (i = 0U; i < NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS; i++)
    {
        if (transmit_messages_infos[i].transmit_message_details != NULL &&
                transmit_messages_infos[i].transmit_message_details->port == port)
        {
            transmit_messages_infos[i].current_transmit_period_ms = (uint16_t)(((uint32_t)transmit_messages_infos[i].current_transmit_period_ms * permil_period_adjustment) / 1000UL);

            if (transmit_messages_infos[i].current_transmit_period_ms < transmit_messages_infos[i].transmit_message_details->transmit_period_ms)
            {
                transmit_messages_infos[i].current_transmit_period_ms = transmit_messages_infos[i].transmit_message_details->transmit_period_ms;
            }
        }
    }
}

/**
 * Find a receive message details from a port and a message type
 *
 * @param port The port to search for
 * @param message_type The message type to search for
 * @return A pointer to the found receive message details structure or NULL if not found
 */
static const nmea_receive_message_details_t *get_receive_message_details(uint8_t port, nmea_message_type_t message_type)
{
   uint16_t i;

   for (i = 0U; i < NMEA_MAXIMUM_RECEIVE_MESSAGE_DETAILS; i++)
   {
       if (receive_message_details[i] != NULL)
       {
           if (receive_message_details[i]->port == port &&
                   receive_message_details[i]->message_type == message_type)
           {
               return receive_message_details[i];
           }
       }
   }

   return NULL;
}

/**
 * Find a transmit message details from a port and a message type
 *
 * @param port The port to search for
 * @param messageType The message type to search for
 * @return A pointer to the found transmit message details structure or NULL if not found
 */
static const transmit_message_details_t *get_transmit_message_details(uint8_t port, nmea_message_type_t message_type)
{
   uint16_t i;

   for (i = 0U; i < NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS; i++)
   {
       if (transmit_messages_infos[i].transmit_message_details != NULL)
       {
           if (transmit_messages_infos[i].transmit_message_details->port == port &&
                   transmit_messages_infos[i].transmit_message_details->message_type == message_type)
           {
               return transmit_messages_infos[i].transmit_message_details;
           }
       }
   }

   return NULL;
}

/**
 * Check a received message for basic syntax correctness - size, comma count, length after first comma
 *
 * @param message_data The message to check
 * @param min_commas Minimum commas expected
 * @param max_commas Maximum commas expected
 * @return If check is ok true else false
 */
static bool check_received_message(const char *message_data, uint8_t min_commas, uint8_t max_commas)
{
	size_t length = strlen(message_data);
	uint8_t comma_count = count_commas(message_data);

	if (length < (size_t)NMEA_MIN_MESSAGE_LENGTH || message_data[length - (size_t)2] != '\r')
	{
		return false;
	}

	if (comma_count < min_commas || comma_count > max_commas)
	{
		return false;
	}

    if (strlen(my_strtok(message_data, ",")) != (size_t)6)
    {
        return false;
    }

	return true;
}

/**
 * Send data to a serial port
 * 
 * @param port The port to send data to
 * @param data_size Bytes in the buffer of data to send
 * @param data Buffer holding data to send
 * @param data_sent Pointer to variable that will hold the number of bytes actually sent
 * @return A standard library error code 
 */
static nmea_error_t send_data(uint8_t port, uint16_t data_size, const uint8_t *data, uint16_t *data_sent)
{
	switch (port)
	{
	case 0U:
        *data_sent = serial_1_send_data(data_size, data);	
		break;	
		
	case 1U:
        *data_sent = serial_2_send_data(data_size, data);	
		break;			

	default:
		break;
	}

    if (*data_sent != data_size)
    {
        return nmea_error_overflow;
    }

    return nmea_error_none;
}

/**
 * Read data from a serial port
 * 
 * @param port The port to send data to
 * @param buffer_length Length of buffer to read data into
 * @param data Buffer holding data to write read data into
 * @return Bytes read 
 */
static uint16_t receive_data(uint8_t port, uint16_t buffer_length, uint8_t *data)
{
	uint16_t bytes_read = 0U;
	switch (port)
	{
	case 0U:
		bytes_read = serial_1_read_data(buffer_length, data);
		break;

	case 1U:
        bytes_read = serial_2_read_data(buffer_length, data);
        break;

	default:
		break;
	}

	return bytes_read;
}

/**
 * Perform a safe equivalent of strcat
 * 
 * @param dest Buffer to hold the concatenated string
 * @param size Size in bytes of dest
 * @param src The source string to append to what already exists in dest
 * @return true if string concatenated successfully
 */
static bool util_safe_strcat(char *dest, size_t size, const char *src)
{
    if (dest == NULL || src == NULL || (strlen(dest) + strlen(src) + (size_t)1 > size))
    {
    	return false;
    }

	return (strncat(dest, src, (size - strlen(dest) - (size_t)1U)));
}

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void nmea_process(void)
{
    volatile uint32_t time_ms;
    uint16_t i;
    uint8_t port;
    char message_buffer[NMEA_MAX_MESSAGE_LENGTH + 1];
    uint16_t data_sent;
    nmea_error_t send_error;
    uint8_t bytes_to_read;
    uint16_t bytes_read;
    uint8_t unread_length;
    uint8_t bytes_used;
    uint16_t bytes_to_move;
    uint32_t oldest_message_time;
    uint16_t message_needs_transmitting;
    transmit_message_info_t *transmit_details_info;
    transmit_message_info_t *oldest_message_details_info;
    uint16_t message_length;

    for (port = 0U; port < NMEA_NUMBER_OF_PORTS; port++)
    {
    	message_length = (uint16_t)strlen(&message_data_to_send_buffer[port][0]);
        if (message_length > 0U)
        {
            send_error = send_data(port,
            		message_length,
                    (uint8_t*)&message_data_to_send_buffer[port][0],
                    &data_sent);
            if (send_error == nmea_error_none)
            {
                message_data_to_send_buffer[port][0] = '\0';
            }
            else
            {
                bytes_to_move = message_length - data_sent;
                (void)memmove(&message_data_to_send_buffer[port][0],
                        &message_data_to_send_buffer[port][0] + data_sent,
                        (size_t)bytes_to_move);
                message_data_to_send_buffer[port][bytes_to_move] = '\0';
            }
        }
    }

	time_ms = timer_get_time_ms();

    for (port = 0U; port < NMEA_NUMBER_OF_PORTS; port++)
    {
        send_error = nmea_error_none;

        if (strlen(&message_data_to_send_buffer[port][0]) > (size_t)0)
        {
            continue;
        }

        for (i = 0U; i < NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS; i++)
        {
        	transmit_details_info = get_transmit_message_info(i);

            if (transmit_details_info != NULL &&
            		transmit_details_info->transmit_message_details != NULL &&
					transmit_details_info->transmit_now &&
					transmit_details_info->transmit_message_details->port == port)
            {
            	transmit_details_info->transmit_now = false;

                if (encode(transmit_details_info, message_buffer) == nmea_error_none)
                {
                    send_error = send_data(port, (uint16_t)strlen(message_buffer), (uint8_t*)message_buffer, &data_sent);
                    if (send_error != nmea_error_none)
                    {
                       	(void)strcpy(&message_data_to_send_buffer[port][0], message_buffer + data_sent);
                        break;
                    }
                }
            }
        }

        if (send_error != nmea_error_none)
        {
            continue;
        }

        do
        {
            oldest_message_time = UINT32_MAX;
            message_needs_transmitting = false;

            for (i = 0U; i < NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS; i++)
            {
            	transmit_details_info = get_transmit_message_info(i);

                if (transmit_details_info != NULL &&
                		transmit_details_info->transmit_message_details != NULL &&
						transmit_details_info->transmit_message_details->transmit_period_ms > 0UL &&
						transmit_details_info->transmit_message_details->port == port &&
						time_ms >= transmit_details_info->next_transmit_time)
                {
                    if (transmit_details_info->next_transmit_time < oldest_message_time)
                    {
                    	oldest_message_details_info = transmit_details_info;
                        oldest_message_time = transmit_details_info->next_transmit_time;
                        message_needs_transmitting = true;
                    }
                }
            }

            if (message_needs_transmitting)
            {
                if (encode(oldest_message_details_info, message_buffer) == nmea_error_none)
                {
                    send_error = send_data(port, (uint16_t)strlen(message_buffer), (uint8_t*)message_buffer, &data_sent);
                    oldest_message_details_info->next_transmit_time = time_ms + oldest_message_details_info->current_transmit_period_ms;

                    if (send_error != nmea_error_none)
                    {
                        if (send_error == nmea_error_overflow)
                        {
                        	(void)strcpy(&message_data_to_send_buffer[port][0], message_buffer + data_sent);
                        	adjust_messages_speed(port, NMEA_SLOW_DOWN_MESSAGE_PERMIL_PERIOD_ADJUSTMENT);
                        }
                        break;
                    }
                }
                else
                {
                	oldest_message_details_info->next_transmit_time = time_ms + oldest_message_details_info->current_transmit_period_ms;
                }

            }
        } while (message_needs_transmitting && send_error == nmea_error_none);

        if (send_error != nmea_error_none)
        {
            continue;
        }

        adjust_messages_speed(port, NMEA_SPEED_UP_MESSAGE_PERMIL_PERIOD_ADJUSTMENT);
    }

    for (port = 0U; port < NMEA_NUMBER_OF_PORTS; port++)
    {
        do
        {
            unread_length = (uint8_t)strlen(&message_data_to_read_buffer[port][0]);
            bytes_to_read = NMEA_MAX_MESSAGE_LENGTH - unread_length;
            bytes_read = receive_data(port, bytes_to_read, (uint8_t*)&message_data_to_read_buffer[port][unread_length]);
			if (bytes_read > 0U)
			{
				message_data_to_read_buffer[port][unread_length + bytes_read] = 0;
				bytes_used = decode(&message_data_to_read_buffer[port][0], port);

				if (bytes_used > 0U)
				{
					(void)memmove(&message_data_to_read_buffer[port][0],
							&message_data_to_read_buffer[port][bytes_used],
							(size_t)((NMEA_MAX_MESSAGE_LENGTH + 1) - bytes_used));
				}
			}
        } while (bytes_to_read == bytes_read);
    }
}

void nmea_disable_transmit_message(uint8_t port, nmea_message_type_t message_type)
{
    uint16_t i;

    for (i = 0U; i < NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS; i++)
    {
        if (transmit_messages_infos[i].transmit_message_details != NULL &&
                transmit_messages_infos[i].transmit_message_details->port == port &&
                transmit_messages_infos[i].transmit_message_details->message_type == message_type)
        {
            transmit_messages_infos[i].transmit_message_details = NULL;
            return;
        }
    }
}

void nmea_enable_transmit_message(const transmit_message_details_t *nmea_transmit_message_details)
{
    uint16_t i;

    for (i = 0U; i < NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS; i++)
    {
        if (transmit_messages_infos[i].transmit_message_details != NULL &&
                transmit_messages_infos[i].transmit_message_details->port == nmea_transmit_message_details->port &&
                transmit_messages_infos[i].transmit_message_details->message_type == nmea_transmit_message_details->message_type)
        {
            return;
        }
    }

    if (nmea_transmit_message_details->message_type >= nmea_message_max)
    {
        return;
    }

    for (i = 0U; i < NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS; i++)
    {
        if (transmit_messages_infos[i].transmit_message_details == NULL)
        {
            break;
        }
    }

    if (i == NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS)
    {
        return;
    }

    transmit_messages_infos[i].transmit_message_details = nmea_transmit_message_details;
    transmit_messages_infos[i].next_transmit_time = timer_get_time_ms() + nmea_transmit_message_details->transmit_period_ms;
    transmit_messages_infos[i].current_transmit_period_ms = nmea_transmit_message_details->transmit_period_ms;
}

void nmea_enable_receive_message(const nmea_receive_message_details_t *nmea_receive_message_details)
{
    uint16_t i;

    if (nmea_receive_message_details->message_type >= nmea_message_max)
    {
        return;
    }

    for (i = 0U; i < NMEA_MAXIMUM_RECEIVE_MESSAGE_DETAILS; i++)
    {
        if (receive_message_details[i] != NULL &&
                receive_message_details[i]->port == nmea_receive_message_details->port &&
                receive_message_details[i]->message_type == nmea_receive_message_details->message_type)
        {
            return;
        }
    }

    for (i = 0U; i < NMEA_MAXIMUM_RECEIVE_MESSAGE_DETAILS; i++)
    {
        if (receive_message_details[i] == NULL)
        {
            break;
        }
    }

    if (i == NMEA_MAXIMUM_RECEIVE_MESSAGE_DETAILS)
    {
        return;
    }

    receive_message_details[i] = nmea_receive_message_details;
}

void nmea_transmit_message_now(uint8_t port, nmea_message_type_t message_type)
{
    const transmit_message_details_t *transmit_message_details;
    uint16_t i;

    transmit_message_details = get_transmit_message_details(port, message_type);
    if (transmit_message_details == NULL)
    {
        return;
    }

    for (i = 0U; i < NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS; i++)
    {
        if (transmit_messages_infos[i].transmit_message_details == transmit_message_details)
        {
            transmit_messages_infos[i].transmit_now = true;
            break;
        }
    }
}

nmea_error_t nmea_encode_DPT(char *message_data, const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    const nmea_message_data_DPT_t *source_DPT;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_DPT = (nmea_message_data_DPT_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIDPT,");

    if (source_DPT->data_available & NMEA_DPT_DEPTH_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_DPT->depth), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_DPT->data_available & NMEA_DPT_DEPTH_OFFSET_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_DPT->depth_offset), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

	if (source_DPT->data_available & NMEA_DPT_DEPTH_MAX_RANGE_PRESENT)
	{
		next_field_data = my_ftoa((float)(source_DPT->depth_maximum_range), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
	}

    return nmea_error_none;
}

nmea_error_t nmea_encode_HDM(char *message_data, const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    const nmea_message_data_HDM_t *source_HDM;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_HDM = (nmea_message_data_HDM_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIHDM,");

    if (source_HDM->data_available & NMEA_HDM_MAG_HEADING_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_HDM->magnetic_heading), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",M"))
    {
    	return nmea_error_message;
    }

    return nmea_error_none;
}

nmea_error_t nmea_encode_HDT(char *message_data, const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    const nmea_message_data_HDT_t *source_HDT;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_HDT = (nmea_message_data_HDT_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIHDT,");

    if (source_HDT->data_available & NMEA_HDT_TRUE_HEADING_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_HDT->true_heading), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",T"))
    {
    	return nmea_error_message;
    }

    return nmea_error_none;
}

nmea_error_t nmea_encode_MTW(char *message_data, const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    const nmea_message_data_MTW_t *source_MTW;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_MTW = (nmea_message_data_MTW_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIMTW,");

    if (source_MTW->data_available & NMEA_MTW_WATER_TEMPERATURE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MTW->water_temperature), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",C"))
    {
    	return nmea_error_message;
    }

    return nmea_error_none;
}

nmea_error_t nmea_encode_XDR(char *message_data, const void *source)
{
    uint8_t max_message_length;
    uint8_t tuple;
    const char *next_field_data;
    const nmea_message_data_XDR_t *source_XDR;
    size_t length;
    uint8_t measurements_count;
    uint32_t measurements_mask;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_XDR = (nmea_message_data_XDR_t *)source;

    measurements_count = nmea_count_set_bits(source_XDR->data_available,
            0U,
            NMEA_XDR_MAX_MEASUREMENTS_COUNT);
    if (measurements_count < 1U || measurements_count > NMEA_XDR_MAX_MEASUREMENTS_COUNT)
    {
        return nmea_error_param;
    }

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIXDR");

    measurements_mask = 0x00000001UL;
    for (tuple = 0U; tuple < NMEA_XDR_MAX_MEASUREMENTS_COUNT; tuple++)
    {
        if ((source_XDR->data_available & measurements_mask) == 0UL)
        {
            measurements_mask <<= 1;
            continue;
        }
        measurements_mask <<= 1;

        length = strlen(message_data);
        if (length + (size_t)3 > max_message_length)
        {
            return nmea_error_message;
        }
        (void)strcat(message_data, ",");
        message_data[length + 1] = source_XDR->measurements[tuple].transducer_type;
        message_data[length + 2] = '\0';
        (void)strcat(message_data, ",");

        next_field_data = my_ftoa((float)(source_XDR->measurements[tuple].measurement), source_XDR->measurements[tuple].decimal_places, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
        if (!util_safe_strcat(message_data, max_message_length, ","))
        {
        	return nmea_error_message;
        }

        length = strlen(message_data);
        if (length + (size_t)2 > max_message_length)
        {
            return nmea_error_message;
        }
        message_data[length] = source_XDR->measurements[tuple].units;
        message_data[length + (size_t)1] = '\0';
        (void)strcat(message_data, ",");

        if (!util_safe_strcat(message_data, max_message_length, source_XDR->measurements[tuple].transducer_id))
        {
        	return nmea_error_message;
        }
    }

    return nmea_error_none;
}

nmea_error_t nmea_encode_VLW(char *message_data, const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    const nmea_message_data_VLW_t *source_VLW;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_VLW = (nmea_message_data_VLW_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIVLW,");

    if (source_VLW->data_available & NMEA_VLW_TOTAL_WATER_DISTANCE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_VLW->total_water_distance), 2U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",N,"))
    {
    	return nmea_error_message;
    }

    if (source_VLW->data_available & NMEA_VLW_TRIP_WATER_DISTANCE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_VLW->trip_water_distance), 2U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",N,"))
    {
    	return nmea_error_message;
    }

	if (source_VLW->data_available & NMEA_VLW_TOTAL_GROUND_DISTANCE_PRESENT)
	{
		next_field_data = my_ftoa((float)(source_VLW->total_ground_distance), 2U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
	}
    if (!util_safe_strcat(message_data, max_message_length, ",N,"))
    {
    	return nmea_error_message;
    }

	if (source_VLW->data_available & NMEA_VLW_TRIP_GROUND_DISTANCE_PRESENT)
	{
		next_field_data = my_ftoa((float)(source_VLW->trip_ground_distance), 2U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
	}
    if (!util_safe_strcat(message_data, max_message_length, ",N"))
    {
    	return nmea_error_message;
    }

    return nmea_error_none;
}

nmea_error_t nmea_encode_VHW(char *message_data,  const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    const nmea_message_data_VHW_t *source_VHW;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_VHW = (nmea_message_data_VHW_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIVHW,");

    if (source_VHW->data_available & NMEA_VHW_HEADING_TRUE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_VHW->heading_true), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",T,"))
    {
    	return nmea_error_message;
    }

    if (source_VHW->data_available & NMEA_VHW_HEADING_MAG_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_VHW->heading_magnetic), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",M,"))
    {
    	return nmea_error_message;
    }

    if (source_VHW->data_available & NMEA_VHW_WATER_SPEED_KTS_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_VHW->water_speed_knots), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",N,"))
    {
    	return nmea_error_message;
    }

    if (source_VHW->data_available & NMEA_VHW_WATER_SPEED_KMPH_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_VHW->water_speed_kmph), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",K"))
    {
    	return nmea_error_message;
    }

    return nmea_error_none;
}

nmea_error_t nmea_encode_MWD(char *message_data, const void *source)
{
    uint8_t  max_message_length;
    const char *next_field_data;
    const nmea_message_data_MWD_t *source_MWD;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_MWD = (nmea_message_data_MWD_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIMWD,");

    if (source_MWD->data_available & NMEA_MWD_WIND_DIRECTION_TRUE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MWD->wind_direction_true), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",T,"))
    {
    	return nmea_error_message;
    }

    if (source_MWD->data_available & NMEA_MWD_WIND_DIRECTION_MAG_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MWD->wind_direction_magnetic), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",M,"))
    {
    	return nmea_error_message;
    }

    if (source_MWD->data_available & NMEA_MWD_WIND_SPEED_KTS_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MWD->wind_speed_knots), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",N,"))
    {
    	return nmea_error_message;
    }

    if (source_MWD->data_available & NMEA_MWD_WIND_SPEED_MPS_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MWD->wind_speed_mps), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",M"))
    {
    	return nmea_error_message;
    }

    return nmea_error_none;
}

nmea_error_t nmea_encode_MWV(char *message_data, const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    size_t length;
    const nmea_message_data_MWV_t *source_MWV;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_MWV = (nmea_message_data_MWV_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIMWV,");

    if (source_MWV->data_available & NMEA_MWV_WIND_ANGLE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MWV->wind_angle), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_MWV->data_available & NMEA_MWV_REFERENCE_PRESENT)
    {
        length = strlen(message_data);
        if (length + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }
        message_data[length] = source_MWV->reference;
        message_data[length + (size_t)1] = 0;
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_MWV->data_available & NMEA_MWV_WIND_SPEED_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MWV->wind_speed), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_MWV->data_available & NMEA_MWV_WIND_SPEED_UNITS_PRESENT)
    {
        length = strlen(message_data);
        if (length + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }
        message_data[length] = source_MWV->wind_speed_units;
        message_data[length + (size_t)1] = 0;
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_MWV->data_available & NMEA_MWV_STATUS_PRESENT)
    {
        length = strlen(message_data);
        if (length + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }
        message_data[length] = source_MWV->status;
        message_data[length + (size_t)1] = 0;
    }

    return nmea_error_none;
}

nmea_error_t nmea_decode_VDM(const char *message_data, nmea_message_data_VDM_t *result)
{
	/* sample messages
	!AIVDM,2,1,3,B,55P5TL01VIaAL@7WKO@mBplU@<PDhh000000001S;AJ::4A80?4i@E53,0*3E
	!AIVDM,2,2,3,B,1@0000000000000,2*55
	 */

    const char *next_token;
    uint32_t data_available = 0UL;

    if (!check_received_message(message_data, 6U, 6U))
    {
    	return nmea_error_message;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
    	result->fragment_count = (uint8_t)atoi(next_token);
    	data_available |= NMEA_VDM_FRAGMENT_COUNT_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
    	result->fragment_number = (uint8_t)atoi(next_token);
    	data_available |= NMEA_VDM_FRAGMENT_NUMBER_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
    	result->message_identifier = (uint8_t)atoi(next_token);
    	data_available |= NMEA_VDM_MESSAGE_IDENTIFIER_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
    	result->channel_code = *next_token;
    	data_available |= NMEA_VDM_CHANNEL_CODE_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0 && strlen(next_token) <= (size_t)NMEA_VDM_MAX_AIS_DATA_FIELD_LENGTH)
    {
    	(void)strcpy(result->data, next_token);
    	data_available |= NMEA_VDM_DATA_PRESENT;
    }

    next_token = my_strtok(NULL, "*/r");
    if (strlen(next_token) > (size_t)0)
    {
    	result->fill_bits = (uint8_t)atoi(next_token);
    	data_available |= NMEA_VDM_FILL_BITS_PRESENT;
    }

    result->data_available = data_available;

    return nmea_error_none;
}

nmea_error_t nmea_encode_VDM(char *message_data, const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    const nmea_message_data_VDM_t *source_VDM;
    size_t length;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_VDM = (nmea_message_data_VDM_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "!AIVDM,");

    if (source_VDM->data_available & NMEA_VDM_FRAGMENT_COUNT_PRESENT)
    {
        next_field_data = my_itoa((int32_t)(source_VDM->fragment_count));
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_VDM->data_available & NMEA_VDM_FRAGMENT_NUMBER_PRESENT)
    {
        next_field_data = my_itoa((int32_t)(source_VDM->fragment_number));
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_VDM->data_available & NMEA_VDM_MESSAGE_IDENTIFIER_PRESENT)
    {
        next_field_data = my_itoa((int32_t)(source_VDM->message_identifier));
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_VDM->data_available & NMEA_VDM_CHANNEL_CODE_PRESENT)
    {
        length = strlen(message_data);
        if (length + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }
        message_data[length] = source_VDM->channel_code;
        message_data[length + (size_t)1] = 0;
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_VDM->data_available & NMEA_VDM_DATA_PRESENT)
    {
        if (!util_safe_strcat(message_data, max_message_length, source_VDM->data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_VDM->data_available & NMEA_VDM_FILL_BITS_PRESENT)
    {
        next_field_data = my_itoa((int32_t)(source_VDM->fill_bits));
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (strlen(message_data) + (size_t)1 > (size_t)max_message_length)
    {
        return nmea_error_message;
    }

    return nmea_error_none;
}

nmea_error_t nmea_decode_RMC(const char *message_data, nmea_message_data_RMC_t *result)
{
    const char *next_token;
    uint8_t comma_count = count_commas(message_data);
    uint32_t data_available = 0UL;

    if (!check_received_message(message_data, 11U, 13U))
    {
    	return nmea_error_message;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->utc.hours = (uint8_t)((float)atof(next_token) / 10000.0f);
        result->utc.minutes = (uint8_t)(((float)atof(next_token) - (float)(result->utc.hours * 10000.0f)) / 100.0f);
        result->utc.seconds = (float)atof(next_token) - (float)(result->utc.hours * 10000.0f) - (float)(result->utc.minutes * 100.0f);
        data_available |= NMEA_RMC_UTC_PRESENT;
    }

    next_token=my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->status = *next_token;
        data_available |= NMEA_RMC_STATUS_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->latitude = atof(next_token);
        data_available |= NMEA_RMC_LATITUDE_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (*next_token == 'S')
    {
        result->latitude = -result->latitude;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->longitude = atof(next_token);
        data_available |= NMEA_RMC_LONGITUDE_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (*next_token == 'W')
    {
        result->longitude = -result->longitude;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->SOG = (float)atof(next_token);
        data_available |= NMEA_RMC_SOG_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->COG = (float)atof(next_token);
        data_available |= NMEA_RMC_COG_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->date.date = (uint8_t)(atoi(next_token) / 10000);
        result->date.month = (uint8_t)((atoi(next_token) - (uint32_t)(result->date.date) * 10000UL) / 100UL);
        result->date.year = (uint16_t)(atoi(next_token) - (uint32_t)(result->date.date) * 10000UL - (uint32_t)(result->date.month) * 100UL);
        result->date.year += 2000U;
        data_available |= NMEA_RMC_DATE_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->magnetic_variation = (float)atof(next_token);
        data_available |= NMEA_RMC_MAG_VARIATION_PRESENT;
    }

    next_token = my_strtok(NULL, ",*\r");
    if (strlen(next_token) > (size_t)0)
    {
        result->magnetic_variation_direction = *next_token;
        data_available |= NMEA_RMC_MAG_DIRECTION_PRESENT;
    }

    if (comma_count >= 12U)
    {
        next_token = my_strtok(NULL, ",*\r");
        if (strlen(next_token) > 0U)
        {
            result->mode = *next_token;
            data_available |= NMEA_RMC_MODE_PRESENT;
        }

        if (comma_count == 13U)
        {
            next_token = my_strtok(NULL, "*\r");
            if (strlen(next_token) > (size_t)0)
            {
                result->navigation_status = *next_token;
                data_available |= NMEA_RMC_NAV_STATUS_PRESENT;
            }
        }
    }

    result->data_available = data_available;

    return nmea_error_none;
}

nmea_error_t nmea_encode_RMC(char *message_data, const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    size_t length;
    const nmea_message_data_RMC_t *source_RMC;
    char utc_buffer[12];
    char date_buffer[8];

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_RMC = (nmea_message_data_RMC_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$GPRMC,");

    if (source_RMC->data_available & NMEA_RMC_UTC_PRESENT)
    {
        if (source_RMC->utc.hours < 24U && source_RMC->utc.minutes < 60U && source_RMC->utc.seconds < 60.0f)
        {
            (void)snprintf(utc_buffer, sizeof(utc_buffer), "%02u%02u%04.1f",
                    (unsigned int)(source_RMC->utc.hours),
                    (unsigned int)(source_RMC->utc.minutes),
					source_RMC->utc.seconds);
            if (!util_safe_strcat(message_data, max_message_length, utc_buffer))
            {
            	return nmea_error_message;
            }
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_STATUS_PRESENT)
    {
        length = strlen(message_data);
        if (length + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }
        message_data[length] = source_RMC->status;
        message_data[length + (size_t)1] = 0;
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_LATITUDE_PRESENT)
    {
        next_field_data = my_ftoa(fabsf(source_RMC->latitude), 3U, 4U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_LATITUDE_PRESENT)
    {
        if (strlen(message_data) + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }

        if (source_RMC->latitude < 0.0f)
        {
        	(void)strcat(message_data, "S");
        }
        else
        {
        	(void)strcat(message_data, "N");
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_LONGITUDE_PRESENT)
    {
        next_field_data = my_ftoa(fabsf(source_RMC->longitude), 3U, 5U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_LONGITUDE_PRESENT)
    {
        if (strlen(message_data) + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }

        if (source_RMC->longitude < 0.0f)
        {
            strcat(message_data, "W");
        }
        else
        {
            strcat(message_data, "E");
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_SOG_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_RMC->SOG), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_COG_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_RMC->COG), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_DATE_PRESENT)
    {
        if (source_RMC->date.date < 32U && source_RMC->date.month < 13U && source_RMC->date.year > 2000U && source_RMC->date.year < 2100U)
        {
            (void)snprintf(date_buffer, sizeof(date_buffer), "%02u%02u%02u",
                    (unsigned int)(source_RMC->date.date),
                    (unsigned int)(source_RMC->date.month),
                    (unsigned int)(source_RMC->date.year - 2000U));

            if (!util_safe_strcat(message_data, max_message_length, date_buffer))
            {
            	return nmea_error_message;
            }
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_MAG_VARIATION_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_RMC->magnetic_variation), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_RMC->data_available & NMEA_RMC_MAG_DIRECTION_PRESENT)
    {
        length = strlen(message_data);
        if (length + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }
        message_data[length] = source_RMC->magnetic_variation_direction;
        message_data[length + (size_t)1] = 0;
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

	if (source_RMC->data_available & NMEA_RMC_MODE_PRESENT)
	{
		length = strlen(message_data);
		if (length + (size_t)1 > (size_t)max_message_length)
		{
			return nmea_error_message;
		}
		message_data[length] = source_RMC->mode;
		message_data[length + (size_t)1] = 0;
	}
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

	if (source_RMC->data_available & NMEA_RMC_NAV_STATUS_PRESENT)
	{
		length = strlen(message_data);
		if (length + (size_t)1 > (size_t)max_message_length)
		{
			return nmea_error_message;
		}
		message_data[length] = source_RMC->navigation_status;
		message_data[length + (size_t)1] = '\0';
	}

    return nmea_error_none;
}

nmea_error_t nmea_decode_GGA(const char *message_data, nmea_message_data_GGA_t *result)
{
    const char *next_token;
    uint32_t data_available = 0UL;

    if (!check_received_message(message_data, 14U, 14U))
    {
    	return nmea_error_message;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->utc.hours = (uint8_t)((float)atof(next_token) / 10000.0f);
        result->utc.minutes = (uint8_t)(((float)atof(next_token) - (float)(result->utc.hours * 10000.0f)) / 100.0f);
        result->utc.seconds = (float)atof(next_token) - (float)(result->utc.hours * 10000.0f) - (float)(result->utc.minutes * 100.0f);
        data_available |= NMEA_GGA_UTC_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->latitude = atof(next_token);
        data_available |= NMEA_GGA_LATITUDE_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (*next_token == 'S')
    {
        result->latitude = -result->latitude;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->longitude = atof(next_token);
        data_available |= NMEA_GGA_LONGITUDE_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (*next_token == 'W')
    {
        result->longitude = -result->longitude;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->quality_indicator = (uint8_t)(atoi(next_token));
        data_available |= NMEA_GGA_QUALITY_INDICATOR_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->satellites_in_use = (uint8_t)(atoi(next_token));
        data_available |= NMEA_GGA_SATELLITES_IN_USE_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->HDOP = (float)atof(next_token);
        data_available |= NMEA_GGA_HDOP_PRESENT;
    }

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->altitude = (float)atof(next_token);
        data_available |= NMEA_GGA_ALTITUDE_PRESENT;
    }

    next_token = my_strtok(NULL, ",");

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->geoidal_separation = (float)atof(next_token);
        data_available |= NMEA_GGA_GEIODAL_SEPARATION_PRESENT;
    }

    next_token = my_strtok(NULL, ",");

    next_token = my_strtok(NULL, ",");
    if (strlen(next_token) > (size_t)0)
    {
        result->dgpsAge = (float)atof(next_token);
        data_available |= NMEA_GGA_DGPS_AGE_PRESENT;
    }

    next_token = my_strtok(NULL, "*\r");
    if (strlen(next_token) > (size_t)0)
    {
        result->dgps_station_id = (uint8_t)(atoi(next_token));
        data_available |= NMEA_GGA_DGPS_STATION_ID_PRESENT;
    }

    result->data_available = data_available;

    return nmea_error_none;
}

nmea_error_t nmea_encode_GGA(char *message_data, const void *source)
{
    uint8_t max_message_length;
    const char *next_field_data;
    const nmea_message_data_GGA_t *source_GGA;
    char utc_buffer[12];
    char number_buffer[6];

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_GGA = (nmea_message_data_GGA_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$GPGGA,");

    if (source_GGA->data_available & NMEA_GGA_UTC_PRESENT)
    {
        if (source_GGA->utc.hours < 24U && source_GGA->utc.minutes < 60U && source_GGA->utc.seconds < 60.0f)
        {
            (void)snprintf(utc_buffer, sizeof(utc_buffer), "%02u%02u%04.1f",
                    (unsigned int)(source_GGA->utc.hours),
                    (unsigned int)(source_GGA->utc.minutes),
					source_GGA->utc.seconds);
            if (!util_safe_strcat(message_data, max_message_length, utc_buffer))
            {
            	return nmea_error_message;
            }
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_LATITUDE_PRESENT)
    {
        next_field_data = my_ftoa(fabsf(source_GGA->latitude), 3U, 4U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_LATITUDE_PRESENT)
    {
        if (strlen(message_data) + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }

        if (source_GGA->latitude < 0.0f)
        {
        	(void)strcat(message_data, "S");
        }
        else
        {
        	(void)strcat(message_data, "N");
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_LONGITUDE_PRESENT)
    {
        next_field_data = my_ftoa(fabsf(source_GGA->longitude),3U ,5U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_LONGITUDE_PRESENT)
    {
        if (strlen(message_data) + (size_t)1 > (size_t)max_message_length)
        {
            return nmea_error_message;
        }

        if (source_GGA->longitude < 0.0f)
        {
        	(void)strcat(message_data, "W");
        }
        else
        {
        	(void)strcat(message_data, "E");
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_QUALITY_INDICATOR_PRESENT)
    {
        if (source_GGA->quality_indicator < 9u)
        {
            (void)snprintf(number_buffer, sizeof(number_buffer), "%u", (unsigned int)(source_GGA->quality_indicator));
            if (!util_safe_strcat(message_data, max_message_length, number_buffer))
            {
            	return nmea_error_message;
            }
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_SATELLITES_IN_USE_PRESENT)
    {
        if (source_GGA->satellites_in_use < 13U)
        {
            (void)snprintf(number_buffer, sizeof(number_buffer), "%02u", (unsigned int)(source_GGA->satellites_in_use));
            if (!util_safe_strcat(message_data, max_message_length, number_buffer))
            {
            	return nmea_error_message;
            }
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_HDOP_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_GGA->HDOP), 3U, 0U);		
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_ALTITUDE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_GGA->altitude), 3U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",M,"))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_GEIODAL_SEPARATION_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_GGA->geoidal_separation), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",M,"))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_DGPS_AGE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_GGA->dgpsAge), 0U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_GGA->data_available & NMEA_GGA_DGPS_STATION_ID_PRESENT)
    {
        if (source_GGA->dgps_station_id < 1024U)
        {
            (void)snprintf(number_buffer, sizeof(number_buffer), "%04u", (unsigned int)(source_GGA->dgps_station_id));
            if (!util_safe_strcat(message_data, max_message_length, number_buffer))
            {
            	return nmea_error_message;
            }
        }
    }

    return nmea_error_none;
}

nmea_error_t nmea_encode_MDA(char *message_data, const void *source)
{
    uint8_t  max_message_length;
    const char *next_field_data;
    const nmea_message_data_MDA_t *source_MDA;

    if (message_data == NULL || source == NULL)
    {
        return nmea_error_param;
    }

    source_MDA = (nmea_message_data_MDA_t *)source;

    max_message_length = NMEA_MAX_MESSAGE_LENGTH - 5U;

    (void)strcpy(message_data, "$IIMDA,");

    if (source_MDA->data_available & NMEA_MDA_PRESSURE_INCHES_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->pressure_inches), 3U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",I,"))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_PRESSURE_BARS_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->pressure_bars), 5U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",B,"))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_AIR_TEMPERATURE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->air_temperature), 2U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",C,"))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_WATER_TEMPERATURE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->water_temperature), 2U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",C,"))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_RELATIVE_HUMIDITY_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->relative_huimidity), 2U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_ABSOLUTE_HUMIDITY_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->absolute_humidity), 2U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ","))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_DEW_POINT_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->dew_point), 2U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",C,"))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_WIND_DIRECTION_TRUE_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->wind_direction_true), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",T,"))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_WIND_DIRECTION_MAGNETIC_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->wind_direction_magnetic), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",M,"))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_WINDSPEED_KNOTS_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->windspeed_knots), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",N,"))
    {
    	return nmea_error_message;
    }

    if (source_MDA->data_available & NMEA_MDA_WINDSPEED_MPS_PRESENT)
    {
        next_field_data = my_ftoa((float)(source_MDA->windspeed_mps), 1U, 0U);
        if (!util_safe_strcat(message_data, max_message_length, next_field_data))
        {
        	return nmea_error_message;
        }
    }
    if (!util_safe_strcat(message_data, max_message_length, ",M"))
    {
    	return nmea_error_message;
    }

    return nmea_error_none;
}
