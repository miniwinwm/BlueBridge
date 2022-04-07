#ifndef MODEM_INTERFACE_H
#define MODEM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include "modem.h"

#define MODEM_INTERFACE_LOG_SERIAL			
#define MODEM_INTERFACE_WAIT_FOREVER       portMAX_DELAY  

typedef enum
{
	MODEM_COMMAND_QUEUE,
	MODEM_RESPONSE_QUEUE
} modem_queue_t;

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
void modem_interface_os_init(void);

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
ModemStatus_t modem_interface_queue_put(modem_queue_t modem_queue, const void *msg_ptr, uint32_t timeout);

/**
 *
 */
ModemStatus_t modem_interface_queue_get(modem_queue_t modem_queue, void *msg_ptr, uint32_t timeout);

/**
 *
 */
ModemStatus_t modem_interface_acquire_mutex(uint32_t timeout);

/**
 *
 */
ModemStatus_t modem_interface_release_mutex(void);

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
