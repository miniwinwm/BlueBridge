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

#include "driver/uart.h"
#include "driver/gpio.h"
#include "serial.h"
#include "spp_acceptor.h"

void serial_init(uint32_t baud_rate_1)
{
	spp_init();

    uart_config_t uart_config = 
    {
        .baud_rate = baud_rate_1,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    uart_driver_install(UART_NUM_2, 2048, 2048, 0, NULL, 0);  
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, GPIO_NUM_17, GPIO_NUM_16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

size_t serial_1_send_data(size_t length, const uint8_t *data)
{
	return (size_t)uart_tx_chars(UART_NUM_2, (const char *)data, (uint32_t)length);
}

size_t serial_1_read_data(size_t buffer_length, uint8_t *data)
{
	size_t bytes_available;
	size_t bytes_read;
	
	uart_get_buffered_data_len(UART_NUM_2, &bytes_available);
	
	if (bytes_available <= buffer_length)
	{
		bytes_read = (size_t)uart_read_bytes(UART_NUM_2, (void *)data, (uint32_t)bytes_available, (TickType_t)1);
	}
	else
	{
		bytes_read = (size_t)uart_read_bytes(UART_NUM_2, (void *)data, (uint32_t)buffer_length, (TickType_t)1);
	}	

	return bytes_read;
}

size_t serial_2_send_data(size_t length, const uint8_t *data)
{
	spp_write(data, length);

	return length;
}

size_t serial_2_read_data(size_t buffer_length, uint8_t *data)
{
	size_t bytes_available;
	size_t bytes_to_read;
	size_t i;
	
	bytes_available = spp_bytes_received_size();
	
	if (bytes_available <= buffer_length)
	{
		bytes_to_read = bytes_available;
	}
	else
	{
		bytes_to_read = buffer_length;
	}	
	
	for (i = (size_t)0; i < bytes_to_read; i++)
	{
		data[i] = spp_read();
	}
	
	return bytes_to_read;
}
