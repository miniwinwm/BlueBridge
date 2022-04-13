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
#include "freertos/queue.h"
#include "esp_log.h"
#include "sms.h"
#include "modem.h"
#include "pdu.h"
#include "util.h"

/****************
*** CONSTANTS ***
****************/

/************
*** TYPES ***
************/

/***********************
*** GLOBAL VARIABLES ***
***********************/

/**********************
*** LOCAL VARIABLES ***
**********************/

static QueueHandle_t sms_waiting_id_queue_handle;

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

static void sms_notification_callback(uint32_t sms_id);

/**********************
*** LOCAL FUNCTIONS ***
**********************/

static void sms_notification_callback(uint32_t sms_id)
{
	ESP_LOGI(pcTaskGetName(NULL), "SMS received notification, SMS Id is %u", sms_id);		
	xQueueSendToBack(sms_waiting_id_queue_handle, (const void *)(&sms_id), (TickType_t)0);	
}

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void sms_init(void)
{
	sms_waiting_id_queue_handle = xQueueCreate((UBaseType_t)10, (UBaseType_t)sizeof(uint32_t));		
	ModemSetSmsNotificationCallback(sms_notification_callback);	
}

bool sms_check_for_new(uint32_t *sms_id)
{
	if (sms_id == NULL)
	{
		return false;
	}
	
	if (xQueueReceive(sms_waiting_id_queue_handle, sms_id, (TickType_t)0) == pdTRUE )		
	{
		return true;
	}

	return false;
}

bool sms_receive(uint32_t sms_id, char *phone_number, size_t phone_number_buffer_length, char *message_text, size_t message_text_buffer_length)
{	
	size_t length;
	uint8_t *pdu_ascii_hex_buf;
	uint8_t pdu_bin_buf[SMS_MAX_PDU_LENGTH];
	time_t receive_time;
	char ascii_hex_byte[3];
	int32_t text_length;
	size_t i;

	pdu_ascii_hex_buf = pvPortMalloc(SMS_MAX_PDU_LENGTH * 2 + 1);
	if (pdu_ascii_hex_buf == NULL)
	{
		return false;
	}
	
	ModemStatus_t modem_status = ModemSmsReceiveMessage(sms_id, &length, pdu_ascii_hex_buf, 1000UL);
	ESP_LOGI(pcTaskGetName(NULL), "ModemSmsReceiveMessage length=%u %s", (uint32_t)length, ModemStatusToText(modem_status));	
	
	if (modem_status == MODEM_OK && length > (size_t)0)
	{
		// convert ascii hex to binary
		ascii_hex_byte[2] = (uint8_t)'\0';
		for (i = (size_t)0; i < length / (size_t)2; i++)
		{
			ascii_hex_byte[0] = pdu_ascii_hex_buf[(unsigned int)i * 2];
			ascii_hex_byte[1] = pdu_ascii_hex_buf[(unsigned int)i * 2 + 1];
			pdu_bin_buf[(unsigned int)i] = (uint8_t)util_htoi(ascii_hex_byte);				
		}

		// decode sms pdu
		text_length = (int32_t)pdu_decode((const unsigned char *)pdu_bin_buf, 
									(int)length / 2, 
									&receive_time, 
									phone_number, 
									(int)phone_number_buffer_length, 
									message_text, 
									(int)message_text_buffer_length);
									
		// parse sms text for key/value pairs
		if (text_length > 0)
		{			
			free(pdu_ascii_hex_buf);
			return true;
		}
		else
		{
			ESP_LOGI(pcTaskGetName(NULL), "SMS PDU decode failed %d", text_length);				
			ESP_LOGI(pcTaskGetName(NULL), "ModemSmsReceiveMessage length=%u %s", (uint32_t)length, ModemStatusToText(modem_status));	
			ESP_LOGI(pcTaskGetName(NULL), "ModemSmsReceiveMessage pdu=%s", pdu_ascii_hex_buf);					
		}		
	}	
	
	vPortFree(pdu_ascii_hex_buf);
	
	return false;
}

bool sms_send(const char *message_text, const char *phone_number)
{
	char *ascii_hex_pdu = NULL;
	uint8_t *binary_pdu = NULL;
	size_t pdu_binary_length;
	size_t i;
	char ascii_hex_byte[3];	
	bool success = false;
	ModemStatus_t modem_status;
	
	if (message_text != NULL && phone_number != NULL)
	{
		ascii_hex_pdu = pvPortMalloc(SMS_MAX_PDU_LENGTH * 2 + 1);
		binary_pdu = pvPortMalloc(SMS_MAX_PDU_LENGTH);	
		
		if (ascii_hex_pdu != NULL && binary_pdu != NULL)
		{
			pdu_binary_length = (int32_t)pdu_encode(NULL, phone_number, message_text, (unsigned char *)binary_pdu, SMS_MAX_PDU_LENGTH);
			if (pdu_binary_length > 0)
			{
				ascii_hex_pdu[0] = '\0';								
				for (i = 0; i < pdu_binary_length; i++)
				{
					(void)sprintf(ascii_hex_byte, "%02x", binary_pdu[i]);
					(void)util_safe_strcat(ascii_hex_pdu, (size_t)(SMS_MAX_PDU_LENGTH * 2), ascii_hex_byte);
				}
				
				modem_status = ModemSmsSendMessage(ascii_hex_pdu, 60000UL);		
				ESP_LOGI(pcTaskGetName(NULL), "ModemSmsSendMessage %s", ModemStatusToText(modem_status));	
				success = (modem_status == MODEM_OK);
			}
		}	
		
		vPortFree(ascii_hex_pdu);
		vPortFree(binary_pdu);	
	}		
	
	return success;
}