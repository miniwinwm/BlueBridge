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

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "modem_interface.h"
#include "util.h"

/**************
*** DEFINES ***
**************/

#define MODEM_RESET_GPIO	GPIO_NUM_25
#define MODEM_TX_GPIO		GPIO_NUM_26
#define MODEM_RX_GPIO		GPIO_NUM_27

/************
*** TYPES ***
************/

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

static void modem_interface_task(void *parameters);

/**********************
*** LOCAL VARIABLES ***
**********************/

static TaskHandle_t modem_task_handle;
static QueueHandle_t commandQueueHandle;
static QueueHandle_t responseQueueHandle;
static SemaphoreHandle_t modemMutexHandle;
static modem_task_t modem_task;

/***********************
*** GLOBAL VARIABLES ***
***********************/

/****************
*** CONSTANTS ***
****************/

/**********************
*** LOCAL FUNCTIONS ***
**********************/

static void modem_interface_task(void *parameters)
{
	(void)parameters;
	
	modem_task();
}

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void modem_interface_log(const char *message)
{
	ESP_LOGI(pcTaskGetName(NULL), "%s", message);
}

void modem_interface_os_init(size_t command_queue_packet_size, size_t response_queue_command_size, modem_task_t task)
{
	modem_task = task;
	modemMutexHandle = xSemaphoreCreateMutex();
	commandQueueHandle = xQueueCreate((UBaseType_t)10, (UBaseType_t)command_queue_packet_size);
	responseQueueHandle = xQueueCreate((UBaseType_t)10, (UBaseType_t)response_queue_command_size);
    (void)xTaskCreate(modem_interface_task, "modem task", (configSTACK_DEPTH_TYPE)16384, NULL, (UBaseType_t)0, &modem_task_handle); 
}

void modem_interface_os_deinit(void)
{
	vTaskDelete(modem_task_handle);
	vSemaphoreDelete(modemMutexHandle);
	vQueueDelete(commandQueueHandle);
	vQueueDelete(responseQueueHandle);	
	modem_task_handle = NULL;
	modemMutexHandle = NULL;
	commandQueueHandle = NULL;
	responseQueueHandle = NULL;
}

void modem_interface_serial_init(void)
{
	const int uart_buffer_size = 2048;
	
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };	
	
    (void)uart_driver_install(UART_NUM_1, uart_buffer_size, uart_buffer_size, 0, NULL, 0);
	(void)uart_param_config(UART_NUM_1, &uart_config);
	(void)uart_set_pin(UART_NUM_1, MODEM_TX_GPIO, MODEM_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void modem_interface_serial_close(void)
{
	if (uart_is_driver_installed(UART_NUM_1))
	{
		(void)uart_driver_delete(UART_NUM_1);
	}
}

size_t modem_interface_serial_received_bytes_waiting(void)
{
	size_t size;
	
	(void)uart_get_buffered_data_len(UART_NUM_1, &size);
	
	return size;
}

size_t modem_interface_serial_read_data(size_t buffer_length, uint8_t *data)
{
	size_t size;
	
	size = uart_read_bytes(UART_NUM_1, data, (uint32_t)buffer_length, (TickType_t )0);

#ifdef MODEM_INTERFACE_LOG_SERIAL
	static uint8_t debug_buffer[1024];
	static size_t debug_buffer_length = 0;
	
	if (size + debug_buffer_length + 1 > sizeof(debug_buffer))
	{
		// overflow, give up
		debug_buffer_length = 0;
	}
	else
	{
		(void)memcpy(debug_buffer + debug_buffer_length, data, size);
		debug_buffer_length += size;
		if (debug_buffer_length > 0 && debug_buffer[debug_buffer_length - 1] == '\n')
		{
util_replace_char((char *)debug_buffer,'\r', 'r');
util_replace_char((char *)debug_buffer,'\n', 'n');
			
			debug_buffer[debug_buffer_length] = '\0';
			modem_interface_log((const char *)debug_buffer);
			debug_buffer_length = 0;
		}
	}
#endif
	
	return size;
}

size_t modem_interface_serial_write_data(size_t length, const uint8_t *data)
{
	return (size_t)uart_write_bytes(UART_NUM_1, data, length);
}

void modem_interface_task_delay(uint32_t delay_ms)
{
	TickType_t ticks = (TickType_t)(delay_ms / portTICK_PERIOD_MS);
	if (ticks < (TickType_t)1)
	{
		ticks = (TickType_t)1;
	}
		
	vTaskDelay(ticks);
}

uint32_t modem_interface_get_time_ms(void) 
{
  return (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
}

modem_interface_status_t modem_interface_queue_put(modem_interface_queue_t modem_interface_queue, const void *msg_ptr, uint32_t timeout) 
{
	modem_interface_status_t modem_interface_status = MODEM_INTERFACE_OK;
	QueueHandle_t mq_id;
	
	if (modem_interface_queue == MODEM_INTERFACE_COMMAND_QUEUE)
	{
		mq_id = commandQueueHandle;
	}
	else if (modem_interface_queue == MODEM_INTERFACE_RESPONSE_QUEUE)
	{
		mq_id = responseQueueHandle;
	}
	else
	{
		return MODEM_INTERFACE_ERROR;
	}
	
	if (xQueueSendToBack(mq_id, msg_ptr, (TickType_t)timeout) != pdPASS) 
	{
		if (timeout != 0UL) 
		{
			modem_interface_status = MODEM_INTERFACE_TIMEOUT;
		} 
		else 
		{
			modem_interface_status = MODEM_INTERFACE_ERROR;
		}	
	}
	
	return modem_interface_status;
}

modem_interface_status_t modem_interface_queue_get(modem_interface_queue_t modem_interface_queue, void *msg_ptr, uint32_t timeout) 
{
	modem_interface_status_t modem_interface_status = MODEM_INTERFACE_OK;
	QueueHandle_t mq_id;
	
	if (modem_interface_queue == MODEM_INTERFACE_COMMAND_QUEUE)
	{
		mq_id = commandQueueHandle;
	}
	else if (modem_interface_queue == MODEM_INTERFACE_RESPONSE_QUEUE)
	{
		mq_id = responseQueueHandle;
	}
	else
	{
		return MODEM_INTERFACE_ERROR;
	}	
  
	if (xQueueReceive(mq_id, msg_ptr, (TickType_t)timeout) != pdPASS)
	{
		if (timeout != 0UL) 
		{
			modem_interface_status = MODEM_INTERFACE_TIMEOUT;
		} 
		else 
		{
			modem_interface_status = MODEM_INTERFACE_ERROR;
		}
	}

	return modem_interface_status;
}

modem_interface_status_t modem_interface_acquire_mutex(uint32_t timeout) 
{
	modem_interface_status_t modem_interface_status = MODEM_INTERFACE_OK;

	if (xSemaphoreTake(modemMutexHandle, (TickType_t)timeout) != pdPASS) 
	{
		if (timeout != 0UL) 
		{
			modem_interface_status = MODEM_INTERFACE_TIMEOUT;
		} 
		else 
		{
			modem_interface_status = MODEM_INTERFACE_ERROR;
		}
	}

	return modem_interface_status;
}

modem_interface_status_t modem_interface_release_mutex(void) 
{
	modem_interface_status_t modem_interface_status = MODEM_INTERFACE_OK;

	if (xSemaphoreGive(modemMutexHandle) != pdPASS) 
	{
		modem_interface_status = MODEM_INTERFACE_ERROR;
	}

	return modem_interface_status;
}

void *modem_interface_malloc(size_t length)
{
	return pvPortMalloc(length);
}

void modem_interface_free(void *address)
{
	vPortFree(address);
}


