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
#include "util.h"

/**************
*** DEFINES ***
**************/

#define WAIT_FOREVER       									portMAX_DELAY  			///< Redefinition of FreeRTOS wait forever definition
#define SIGNATURE											0xDEADBEEFUL			///< Settings signature in flash to determine settings have been written previously
#define SETTINGS_DEFAULT_CAN_DEVICE_ADDRESS					22U						///< Default NMEA2000 network address 
#define SETTINGS_DEFAULT_APN								"data.uk"				///< Default APN for the 1p operator
#define SETTINGS_DEFAULT_APN_USER_NAME						"user"					///< Default user name for the 1p operator
#define SETTINGS_DEFAULT_APN_PASSWORD						"one2one"				///< Default password for the 1p operator
#define SETTINGS_DEFAULT_MQTT_BROKER_ADDRESS				"broker.emqx.io"		///< Default MQTT broker ip address
#define SETTINGS_DEFAULT_MQTT_BROKER_PORT					1883U					///< Default MQTT broker port
#define SETTINGS_DEFAULT_MQTT_PUBLISH_PERIOD				30UL					///< Default MQTT publish period in seconds
#define SETTINGS_DEFAULT_MQTT_PUBLISH_START_ON_BOOT			true					///< Default if to start publishing on boot without receiving a start message
#define SETTINGS_DEFAULT_CREATE_TEST_DATA					false					///< Default of if to create test 

/************
*** TYPES ***
************/

/**
 * Settings data structure in flash
 */
typedef struct 
{
    uint32_t signature;																///< Signature to determine if settings have been written previously
    uint8_t device_address;															///< NMEA2000 device address
	char apn[MODEM_MAX_APN_LENGTH + 1];												///< GSM operator APN
	char apn_user_name[MODEM_MAX_USERNAME_LENGTH + 1];								///< GSM operator user name
	char apn_password[MODEM_MAX_PASSWORD_LENGTH + 1];								///< GSM operator password
	char mqtt_broker_address[SETTINGS_MQTT_BROKER_ADDRESS_MAX_LENGTH + 1];			///< MQTT broker address - do not add http://, see default above
	uint16_t mqtt_broker_port;														///< MQTT broker port for plain unencrypted TCP access, not websockets or TLS
	uint32_t period_s;																///< MQTT publish period in seconds
} settings_non_volatile_t;

/**
 * Settings data structure in memory, never persisted
 */
typedef struct
{
	uint32_t hashed_imei;															///< Hashed device IMEI used to identify device via MQTT
	char phone_number[MODEM_MAX_PHONE_NUMBER_LENGTH + 1];							///< SMS sender's phone number in international format
	bool boat_iot_started;															///< If MQTT publishing has started
	bool restart_needed;															///< If the device needs rebooting
	bool publishing_start_needed;													///< If MQTT publishing needs starting
} settings_volatile_t;

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

/**********************
*** LOCAL VARIABLES ***
**********************/

static settings_non_volatile_t settings_non_volatile;								///< Memory copy of non-volatile settings read from flash
static settings_volatile_t settings_volatile;										///< Memory copy of volatile settings never saved to flash
static SemaphoreHandle_t settings_mutex_handle;										///< Mutex handle to ensure settings access thread safety
static bool settings_initialized = false;											///< If the settings driver has been initialised

/***********************
*** GLOBAL VARIABLES ***
***********************/

/****************
*** CONSTANTS ***
****************/

/**********************
*** LOCAL FUNCTIONS ***
**********************/

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void settings_reset(void)
{
	(void)memset(&settings_non_volatile, 0, sizeof(settings_non_volatile_t));
	settings_non_volatile.signature = SIGNATURE;
	settings_non_volatile.device_address = SETTINGS_DEFAULT_CAN_DEVICE_ADDRESS;
	(void)util_safe_strcpy(settings_non_volatile.apn, sizeof(settings_non_volatile.apn), SETTINGS_DEFAULT_APN);
	(void)util_safe_strcpy(settings_non_volatile.apn_user_name, sizeof(settings_non_volatile.apn_user_name), SETTINGS_DEFAULT_APN_USER_NAME);
	(void)util_safe_strcpy(settings_non_volatile.apn_password, sizeof(settings_non_volatile.apn_password), SETTINGS_DEFAULT_APN_PASSWORD);		
	(void)util_safe_strcpy(settings_non_volatile.mqtt_broker_address, sizeof(settings_non_volatile.mqtt_broker_address), SETTINGS_DEFAULT_MQTT_BROKER_ADDRESS);
	settings_non_volatile.mqtt_broker_port = SETTINGS_DEFAULT_MQTT_BROKER_PORT;
	settings_non_volatile.period_s = SETTINGS_DEFAULT_MQTT_PUBLISH_PERIOD;
	flash_store_data((const uint8_t *)&settings_non_volatile, sizeof(settings_non_volatile_t));  	
}

void settings_init(void)
{
	if (settings_initialized)
	{
		return;
	}
	
	settings_initialized = true;
	settings_mutex_handle = xSemaphoreCreateMutex();	
	flash_load_data((uint8_t *)&settings_non_volatile, sizeof(settings_non_volatile_t));
    if (settings_non_volatile.signature != SIGNATURE || settings_non_volatile.period_s == 0UL)
    {
		settings_reset();
    }
	
	settings_volatile.boat_iot_started = SETTINGS_DEFAULT_MQTT_PUBLISH_START_ON_BOOT;
	settings_volatile.restart_needed = false;
	settings_volatile.publishing_start_needed = false;
	settings_volatile.hashed_imei = 0UL;	
#ifdef CREATE_TEST_DATA_CODE		
	settings_volatile.create_test_data = SETTINGS_DEFAULT_CREATE_TEST_DATA;
#endif
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
	flash_store_data((const uint8_t *)&settings_non_volatile, sizeof(settings_non_volatile_t));	
	xSemaphoreGive(settings_mutex_handle);	
}

const char *settings_get_apn(void)
{
	static char apn[MODEM_MAX_APN_LENGTH + 1];
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_non_volatile.apn[0] == '\0')
	{
		(void)util_safe_strcpy(apn, sizeof(apn), "not set");
	}
	else
	{
		(void)util_safe_strcpy(apn, sizeof(apn), settings_non_volatile.apn);
	}
	xSemaphoreGive(settings_mutex_handle);		
	
	return apn;
}

void settings_set_apn(const char *apn)
{
	if (strlen(apn) <= MODEM_MAX_APN_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
		(void)strcpy(settings_non_volatile.apn, apn);			// safe strcpy
		xSemaphoreGive(settings_mutex_handle);	
	}
}

const char *settings_get_apn_user_name(void)
{
	static char apn_user_name[MODEM_MAX_USERNAME_LENGTH + 1];
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_non_volatile.apn_user_name[0] == '\0')
	{
		(void)util_safe_strcpy(apn_user_name, sizeof(apn_user_name), "not set");
	}
	else
	{
		(void)util_safe_strcpy(apn_user_name, sizeof(apn_user_name), settings_non_volatile.apn_user_name);
	}
	xSemaphoreGive(settings_mutex_handle);		
	
	return apn_user_name;	
}

void settings_set_apn_user_name(const char *apn_user_name)
{
	if (strlen(apn_user_name) <= MODEM_MAX_USERNAME_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
		(void)strcpy(settings_non_volatile.apn_user_name, apn_user_name);		// safe strcpy
		xSemaphoreGive(settings_mutex_handle);	
	}	
}

const char *settings_get_apn_password(void)
{
	static char apn_password[MODEM_MAX_PASSWORD_LENGTH + 1];	
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_non_volatile.apn_password[0] == '\0')
	{
		(void)util_safe_strcpy(apn_password, sizeof(apn_password), "not set");
	}
	else
	{
		(void)util_safe_strcpy(apn_password, sizeof(apn_password), settings_non_volatile.apn_password);
	}
	xSemaphoreGive(settings_mutex_handle);	

	return apn_password;
}

void settings_set_apn_password(const char *apn_password)
{
	if (strlen(apn_password) <= MODEM_MAX_PASSWORD_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
		(void)strcpy(settings_non_volatile.apn_password, apn_password);		// safe strcpy
		xSemaphoreGive(settings_mutex_handle);	
	}	
}

const char *settings_get_mqtt_broker_address(void)
{
	static char mqtt_broker_address[SETTINGS_MQTT_BROKER_ADDRESS_MAX_LENGTH + 1];	
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_non_volatile.mqtt_broker_address[0] == '\0')
	{
		(void)util_safe_strcpy(mqtt_broker_address, sizeof(mqtt_broker_address), "not set");
	}
	else
	{	
		(void)util_safe_strcpy(mqtt_broker_address, sizeof(mqtt_broker_address), settings_non_volatile.mqtt_broker_address);
	}
	xSemaphoreGive(settings_mutex_handle);		
	
	return mqtt_broker_address;
}

void settings_set_mqtt_broker_address(const char *mqtt_broker_address)
{
	if (strlen(mqtt_broker_address) <= SETTINGS_MQTT_BROKER_ADDRESS_MAX_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
		(void)strcpy(settings_non_volatile.mqtt_broker_address, mqtt_broker_address);		// safe strcpy
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

uint32_t settings_get_hashed_imei(void)
{
	uint32_t hashed_imei;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	hashed_imei = settings_volatile.hashed_imei;
	xSemaphoreGive(settings_mutex_handle);		
	
	return hashed_imei;
}

void settings_set_hashed_imei(uint32_t hashed_imei)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_volatile.hashed_imei = hashed_imei;
	xSemaphoreGive(settings_mutex_handle);	
}

const char *settings_get_phone_number(void)
{
	static char phone_number[MODEM_MAX_PHONE_NUMBER_LENGTH + 1];
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);	
	if (settings_volatile.phone_number[0] == '\0')
	{
		(void)util_safe_strcpy(phone_number, sizeof(phone_number), "not set");
	}
	else
	{	
		(void)util_safe_strcpy(phone_number, sizeof(phone_number), settings_volatile.phone_number);
	}
	xSemaphoreGive(settings_mutex_handle);		
	
	return phone_number;
}

void settings_set_phone_number(const char *phone_number)
{
	if (strlen(phone_number) <= MODEM_MAX_PHONE_NUMBER_LENGTH)
	{
		xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
		(void)strcpy(settings_volatile.phone_number, phone_number);		// safe strcpy
		xSemaphoreGive(settings_mutex_handle);	
	}	
}

bool settings_get_publishing_started(void)
{
	bool boat_iot_started;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	boat_iot_started = settings_volatile.boat_iot_started;
	xSemaphoreGive(settings_mutex_handle);	
	
	return boat_iot_started;
}

void settings_set_publishing_started(bool boat_iot_started)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_volatile.boat_iot_started = boat_iot_started;
	xSemaphoreGive(settings_mutex_handle);	
}

bool settings_get_reboot_needed(void)
{
	bool restart_needed;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	restart_needed = settings_volatile.restart_needed;
	xSemaphoreGive(settings_mutex_handle);	
	
	return restart_needed;
}

void settings_set_reboot_needed(bool restart_needed)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_volatile.restart_needed = restart_needed;
	xSemaphoreGive(settings_mutex_handle);	
}

uint32_t settings_get_publishing_period_s(void)
{
	uint32_t period_s;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	period_s = settings_non_volatile.period_s;
	xSemaphoreGive(settings_mutex_handle);		
	
	return period_s;
}

void settings_set_publishing_period_s(uint32_t period_s)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_non_volatile.period_s = period_s;
	xSemaphoreGive(settings_mutex_handle);		
}

bool settings_get_publishing_start_needed(void)
{
	bool publishing_start_needed;
	
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	publishing_start_needed = settings_volatile.publishing_start_needed;
	xSemaphoreGive(settings_mutex_handle);	
	
	return publishing_start_needed;
}

void settings_set_publishing_start_needed(bool publishing_start_needed)
{
	xSemaphoreTake(settings_mutex_handle, WAIT_FOREVER);			
	settings_volatile.publishing_start_needed = publishing_start_needed;
	xSemaphoreGive(settings_mutex_handle);	
}
