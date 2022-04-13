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
#include "freertos/semphr.h"
#include "settings.h"
#include "flash.h"
#include "modem.h"

/****************
*** CONSTANTS ***
****************/

#define WAIT_FOREVER       							portMAX_DELAY  
#define SIGNATURE									0xDEADBEEFUL
#define SETTINGS_DEFAULT_CAN_DEVICE_ADDRESS			22U
#define SETTINGS_DEFAULT_APN						"data.uk"
#define SETTINGS_DEFAULT_APN_USER_NAME				"user"
#define SETTINGS_DEFAULT_APN_PASSWORD				"one2one"
#define SETTINGS_DEFAULT_MQTT_BROKER_ADDRESS		"broker.emqx.io"
#define SETTINGS_DEFAULT_MQTT_BROKER_PORT			1883U
#define SETTINGS_DEFAULT_MQTT_PUBLISH_PERIOD		30UL
#define SETTINGS_DEFAULT_MQTT_PUBLISH_START_ON_BOOT	true

/************
*** TYPES ***
************/

typedef struct 
{
    uint32_t signature;
    uint8_t device_address;
	char apn[MODEM_MAX_APN_LENGTH + 1];
	char apn_user_name[MODEM_MAX_USERNAME_LENGTH + 1];
	char apn_password[MODEM_MAX_PASSWORD_LENGTH + 1];
	char mqtt_broker_address[SETTINGS_MQTT_BROKER_ADDRESS_MAX_LENGTH + 1];
	uint16_t mqtt_broker_port;
	uint32_t period_s;
} settings_non_volatile_t;

typedef struct
{
	uint32_t code;	
	char phone_number[MODEM_MAX_PHONE_NUMBER_LENGTH + 1];
	bool boat_iot_started;
	bool restart_needed;
} settings_volatile_t;

/***********************
*** GLOBAL VARIABLES ***
***********************/

/**********************
*** LOCAL VARIABLES ***
**********************/

static settings_non_volatile_t settings_non_volatile;
static settings_volatile_t settings_volatile;
static SemaphoreHandle_t settings_mutex_handle;
static bool init = false;

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

/**********************
*** LOCAL FUNCTIONS ***
**********************/

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void settings_init(void)
{
	if (init)
	{
		return;
	}
	
	init = true;
	settings_mutex_handle = xSemaphoreCreateMutex();	
	flash_load_data((uint8_t *)&settings_non_volatile, sizeof(settings_non_volatile_t));
    if (settings_non_volatile.signature != SIGNATURE)
    {
        memset(&settings_non_volatile, 0, sizeof(settings_non_volatile_t));
        settings_non_volatile.signature = SIGNATURE;
        settings_non_volatile.device_address = SETTINGS_DEFAULT_CAN_DEVICE_ADDRESS;
		strcpy(settings_non_volatile.apn, SETTINGS_DEFAULT_APN);
		strcpy(settings_non_volatile.apn_user_name, SETTINGS_DEFAULT_APN_USER_NAME);
		strcpy(settings_non_volatile.apn_password, SETTINGS_DEFAULT_APN_PASSWORD);		
		strcpy(settings_non_volatile.mqtt_broker_address, SETTINGS_DEFAULT_MQTT_BROKER_ADDRESS);
		settings_non_volatile.mqtt_broker_port = SETTINGS_DEFAULT_MQTT_BROKER_PORT;
 		settings_non_volatile.period_s = SETTINGS_DEFAULT_MQTT_PUBLISH_PERIOD;
		flash_store_data((uint8_t *)&settings_non_volatile, sizeof(settings_non_volatile_t));        
    }
	
	settings_volatile.boat_iot_started = SETTINGS_DEFAULT_MQTT_PUBLISH_START_ON_BOOT;
	settings_volatile.restart_needed = false;
	settings_volatile.code = 0UL;
}

uint8_t settings_get_device_address(void)
{
	return settings_non_volatile.device_address;
}

void settings_set_device_address(uint8_t device_address)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	settings_non_volatile.device_address = device_address;
	xSemaphoreGive(settings_mutex_handle);	
}

void settings_save(void)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	flash_store_data((uint8_t *)&settings_non_volatile, sizeof(settings_non_volatile_t));	
	xSemaphoreGive(settings_mutex_handle);	
}

const char *settings_get_apn(void)
{
	static char apn[MODEM_MAX_APN_LENGTH + 1];
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_non_volatile.apn[0] == '\0')
	{
		strcpy(apn, "not set");
	}
	else
	{
		strcpy(apn, settings_non_volatile.apn);
	}
	xSemaphoreGive(settings_mutex_handle);		
	
	return apn;
}

void settings_set_apn(const char *apn)
{
	if (strlen(apn) <= MODEM_MAX_APN_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
		strcpy(settings_non_volatile.apn, apn);
		xSemaphoreGive(settings_mutex_handle);	
	}
}

const char *settings_get_apn_user_name(void)
{
	static char apn_user_name[MODEM_MAX_USERNAME_LENGTH + 1];
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_non_volatile.apn_user_name[0] == '\0')
	{
		strcpy(apn_user_name, "not set");
	}
	else
	{
		strcpy(apn_user_name, settings_non_volatile.apn_user_name);
	}
	xSemaphoreGive(settings_mutex_handle);		
	
	return apn_user_name;	
}

void settings_set_apn_user_name(const char *apn_user_name)
{
	if (strlen(apn_user_name) <= MODEM_MAX_USERNAME_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
		strcpy(settings_non_volatile.apn_user_name, apn_user_name);
		xSemaphoreGive(settings_mutex_handle);	
	}	
}

const char *settings_get_apn_password(void)
{
	static char apn_password[MODEM_MAX_PASSWORD_LENGTH + 1];	
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_non_volatile.apn_password[0] == '\0')
	{
		strcpy(apn_password, "not set");
	}
	else
	{
		strcpy(apn_password, settings_non_volatile.apn_password);
	}
	xSemaphoreGive(settings_mutex_handle);	

	return apn_password;
}

void settings_set_apn_password(const char *apn_password)
{
	if (strlen(apn_password) <= MODEM_MAX_PASSWORD_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
		strcpy(settings_non_volatile.apn_password, apn_password);
		xSemaphoreGive(settings_mutex_handle);	
	}	
}

const char *settings_get_mqtt_broker_address(void)
{
	static char mqtt_broker_address[SETTINGS_MQTT_BROKER_ADDRESS_MAX_LENGTH + 1];	
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_non_volatile.mqtt_broker_address[0] == '\0')
	{
		strcpy(mqtt_broker_address, "not set");
	}
	else
	{	
		strcpy(mqtt_broker_address, settings_non_volatile.mqtt_broker_address);
	}
	xSemaphoreGive(settings_mutex_handle);		
	
	return mqtt_broker_address;
}

void settings_set_mqtt_broker_address(const char *mqtt_broker_address)
{
	if (strlen(mqtt_broker_address) <= SETTINGS_MQTT_BROKER_ADDRESS_MAX_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
		strcpy(settings_non_volatile.mqtt_broker_address, mqtt_broker_address);
		xSemaphoreGive(settings_mutex_handle);	
	}	
}

uint16_t settings_get_mqtt_broker_port(void)
{
	uint16_t mqtt_broker_port;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	mqtt_broker_port = settings_non_volatile.mqtt_broker_port;
	xSemaphoreGive(settings_mutex_handle);	
	
	return mqtt_broker_port;
}

void settings_set_mqtt_broker_port(uint16_t mqtt_broker_port)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_non_volatile.mqtt_broker_port = mqtt_broker_port;
	xSemaphoreGive(settings_mutex_handle);	
}

uint32_t settings_get_code(void)
{
	uint32_t code;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	code = settings_volatile.code;
	xSemaphoreGive(settings_mutex_handle);		
	
	return code;
}

void settings_set_code(uint32_t code)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_volatile.code = code;
	xSemaphoreGive(settings_mutex_handle);	
}

const char *settings_get_phone_number(void)
{
	static char phone_number[MODEM_MAX_PHONE_NUMBER_LENGTH + 1];
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_volatile.phone_number[0] == '\0')
	{
		strcpy(phone_number, "not set");
	}
	else
	{	
		strcpy(phone_number, settings_volatile.phone_number);
	}
	xSemaphoreGive(settings_mutex_handle);		
	
	return phone_number;
}

void settings_set_phone_number(const char *phone_number)
{
	if (strlen(phone_number) <= MODEM_MAX_PHONE_NUMBER_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
		strcpy(settings_volatile.phone_number, phone_number);
		xSemaphoreGive(settings_mutex_handle);	
	}	
}

bool settings_get_boat_iot_started(void)
{
	bool boat_iot_started;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	boat_iot_started = settings_volatile.boat_iot_started;
	xSemaphoreGive(settings_mutex_handle);	
	
	return boat_iot_started;
}

void settings_set_boat_iot_started(bool boat_iot_started)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_volatile.boat_iot_started = boat_iot_started;
	xSemaphoreGive(settings_mutex_handle);	
}

bool settings_get_restart_needed(void)
{
	bool restart_needed;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	restart_needed = settings_volatile.restart_needed;
	xSemaphoreGive(settings_mutex_handle);	
	
	return restart_needed;
}

void settings_set_restart_needed(bool restart_needed)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_volatile.restart_needed = restart_needed;
	xSemaphoreGive(settings_mutex_handle);	
}

uint32_t settings_get_period_s(void)
{
	uint32_t period_s;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	period_s = settings_non_volatile.period_s;
	xSemaphoreGive(settings_mutex_handle);		
	
	return period_s;
}

void settings_set_period_s(uint32_t period_s)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_non_volatile.period_s = period_s;
	xSemaphoreGive(settings_mutex_handle);		
}