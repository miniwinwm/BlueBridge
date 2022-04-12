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

#ifndef MODEM_INTERFACE_H
#define MODEM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#define MODEM_INTERFACE_LOG_SERIAL			
#define MODEM_INTERFACE_WAIT_FOREVER       portMAX_DELAY  

typedef enum
{
	MODEM_INTERFACE_COMMAND_QUEUE,
	MODEM_INTERFACE_RESPONSE_QUEUE
} modem_interface_queue_t;

typedef enum
{
	MODEM_INTERFACE_OK,
	MODEM_INTERFACE_ERROR,
	MODEM_INTERFACE_TIMEOUT
} modem_interface_status_t;

typedef void (*modem_task_t)(void);

/**
 * Call this once before using the rest of the API
 */
void modem_interface_serial_init(void);

/**
 * 
 */
void modem_interface_serial_close(void);

/**
 *
 */
void modem_interface_os_init(size_t command_queue_packet_size, size_t response_queue_command_size, modem_task_t task);

/**
 *
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
 * @param length Length of buffer pointed to by data
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
 *
 */
void modem_interface_task_delay(uint32_t delay_ms);

/**
 *
 */
uint32_t modem_interface_get_time_ms(void);

/**
 *
 */
modem_interface_status_t modem_interface_queue_put(modem_interface_queue_t modem_interface_queue, const void *msg_ptr, uint32_t timeout);

/**
 *
 */
modem_interface_status_t modem_interface_queue_get(modem_interface_queue_t modem_interface_queue, void *msg_ptr, uint32_t timeout);

/**
 *
 */
modem_interface_status_t modem_interface_acquire_mutex(uint32_t timeout);

/**
 *
 */
modem_interface_status_t modem_interface_release_mutex(void);

/**
 *
 */
void *modem_interface_malloc(size_t length);
 
/**
 *
 */
void modem_interface_free(void *address);

/**
 *
 */
void modem_interface_log(const char *message);

#ifdef __cplusplus
}
#endif

#endif
