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

#ifndef MODEM_INTERFACE_H
#define MODEM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/***************
*** INCLUDES ***
***************/

#include <stdint.h>
#include <stdlib.h>

/**************
*** DEFINES ***
**************/

#define MODEM_INTERFACE_LOG_SERIAL							///< Log serial data received to the ESP32 terminal
#define MODEM_INTERFACE_WAIT_FOREVER       portMAX_DELAY  	///< Redefine FreeRTOS wait forever name to a more readable value

/************
*** TYPES ***
************/

/**
 * Enum to identify which queue is to be referernced in queue handling functions
 */
typedef enum
{
	MODEM_INTERFACE_COMMAND_QUEUE,			///< Reference the modem command queue
	MODEM_INTERFACE_RESPONSE_QUEUE			///< Reference the modem response queue
} modem_interface_queue_t;

/**
 * Modem interface error codes
 */
typedef enum
{
	MODEM_INTERFACE_OK,						///< No modem interface error
	MODEM_INTERFACE_ERROR,					///< Modem interface error other trhan timeout
	MODEM_INTERFACE_TIMEOUT					///< Modem interface timeout error
} modem_interface_status_t;

typedef void (*modem_task_t)(void);			///< Declare a pointer to a function that will call the modem task

/*************************
*** EXTERNAL VARIABLES ***
*************************/

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Initialize the modem interface. Call this once before using the rest of the API
 */
void modem_interface_serial_init(void);

/**
 * Close the modem interface serial port
 */
void modem_interface_serial_close(void);

/**
 * Initialize modem interface operating system provided objects
 *
 * @param command_queue_packet_size Size in bytes of packets on the command queue
 * @param response_queue_command_size Size in bytes of packets on the response queue
 * @param task Pointer to function that implements the modem task
 */
void modem_interface_os_init(size_t command_queue_packet_size, size_t response_queue_command_size, modem_task_t task);

/**
 * Clean up all operating system objects created in modem_interface_os_init()
 */
void modem_interface_os_deinit(void);

/**
 * Write data to UART 0
 *
 * @param length Number of bytes pointed to by data
 * @param data The bytes to send
 * @return How many bytes were transferred into the send buffer which may be less than length if the buffer is full
 */
size_t modem_interface_serial_write_data(size_t length, const uint8_t *data);

/**
 * Read data from UART 0
 *
 * @param buffer_length Length of buffer pointed to by data
 * @param data Buffer to contain the read data
 * @return How many bytes were transferred into the buffer which may be less than length if there were not enough data available
 */
size_t modem_interface_serial_read_data(size_t buffer_length, uint8_t *data);

/**
 * Returns the number of bytes in the receive buffer waiting to be read
 *
 * @return Number of bytes waiting
 */
size_t modem_interface_serial_received_bytes_waiting(void);

/**
 * Delay the calling task by specified time
 * 
 * @param delay_ms Milliseconds to delay the calling task
 */
void modem_interface_task_delay(uint32_t delay_ms);

/**
 * Get the system time since start up in milliseconds
 *
 * @return Time in milliseconds
 */
uint32_t modem_interface_get_time_ms(void);

/**
 * Add an item to one of the modem queues
 *
 * @param modem_interface_queue The queue to add the item to
 * @param msg_ptr The item to add
 * @param timeout The time to wait for space to become available on the queue in operating system ticks where MODEM_INTERFACE_WAIT_FOREVER is the longest possible delay
 * @return Any one of the error codes defined above
 */
modem_interface_status_t modem_interface_queue_put(modem_interface_queue_t modem_interface_queue, const void *msg_ptr, uint32_t timeout);

/**
 * Retrieve an item from one of the modem queues
 *
 * @param modem_interface_queue The queue to retrieve the item from
 * @param msg_ptr A buffer to hold the retrieved item
 * @param timeout The time to wait for an item to become available on the queue in operating system ticks where MODEM_INTERFACE_WAIT_FOREVER is the longest possible delay
 * @return Any one of the error codes defined above */
modem_interface_status_t modem_interface_queue_get(modem_interface_queue_t modem_interface_queue, void *msg_ptr, uint32_t timeout);

/**
 * Get a modem mutex from the operating system
 *
 * @param timeout The time to wait for the mutex to become available
 * @return Any one of the error codes defined above
 */
modem_interface_status_t modem_interface_acquire_mutex(uint32_t timeout);

/**
 * Release a modem mutex
 */
modem_interface_status_t modem_interface_release_mutex(void);

/**
 * Allocate memory using an operating system defined allocation function
 * 
 * @param length
 * @return The allocated memory or NULL is unsuccessful
 */
void *modem_interface_malloc(size_t length);
 
/**
 * Free memory previously allocated by modem_interface_malloc
 *
 * @param address The address to free
 */
void modem_interface_free(void *address);

/**
 * Log a message to an interface defined output
 *
 * @param message String containing the message to log
 */
void modem_interface_log(const char *message);

#ifdef __cplusplus
}
#endif

#endif
