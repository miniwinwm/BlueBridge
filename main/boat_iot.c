#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "main.h"
#include "esp_log.h"
#include "modem.h"
#include "mqtt.h"
#include "property_parser.h"
#include "settings.h"
#include "sms.h"
#include "util.h"

#define NETWORK_REGISTRATION_WAIT_TIME_MS	30000UL					// time to wait for network registration before giving up
// plusnet
//#define ACCESS_POINT_NAME					"everywhere"			// SET YOUR OWN VALUE network settings
//#define USER_NAME							"eesecure"				// SET YOUR OWN VALUE may be blank for some networks in which case change to NULL (not "NULL")
//#define PASSWORD							"secure"				// SET YOUR OWN VALUE may be blank for some networks in which case change to NULL (not "NULL")
// 1p
//#define ACCESS_POINT_NAME					"data.uk"			// SET YOUR OWN VALUE network settings
//#define USER_NAME							"user"				// SET YOUR OWN VALUE may be blank for some networks in which case change to NULL (not "NULL")
//#define PASSWORD							"one2one"				// SET YOUR OWN VALUE may be blank for some networks in which case change to NULL (not "NULL")

#define MQTT_PUBLISH_TOPIC_ROOT				"BluePillDemo"			// SET YOUR OWN VALUE topic root for all published values

static bool modem_start(void);
static bool modem_network_register(void);
static bool modem_set_parameters(void);
static bool modem_activate_data_connection(void);
static bool config_parser_callback(char *key, char *value);

static bool modem_network_register(void)
{
	ModemStatus_t modem_status;
	uint32_t startTime;
	bool registrationStatus;	
	
	ESP_LOGI(pcTaskGetName(NULL), "Attempting to register on network\r\n");
	startTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
	do
	{
		modem_status = ModemGetNetworkRegistrationStatus(&registrationStatus, 250UL);
		vTaskDelay(1000UL / portTICK_PERIOD_MS);

		if ((xTaskGetTickCount() * portTICK_PERIOD_MS) > startTime + NETWORK_REGISTRATION_WAIT_TIME_MS)
		{
			ESP_LOGI(pcTaskGetName(NULL), "Could not register on network\r\n");
			break;
		}
	} 
	while (!registrationStatus);
	
	ESP_LOGI(pcTaskGetName(NULL), "Register on network: %s %u", ModemStatusToText(modem_status), (uint32_t)registrationStatus);	
	
	return registrationStatus;
}

static bool modem_set_parameters(void)
{
	ModemStatus_t modem_status;
	char imei[MODEM_MAX_IMEI_LENGTH + 1] = "";	
	
	modem_status = ModemGetIMEI(imei, MODEM_MAX_IMEI_LENGTH + 1, 1000UL);
	ESP_LOGI(pcTaskGetName(NULL), "IMEI %s %s", imei, ModemStatusToText(modem_status));	
	if (modem_status != MODEM_OK)
	{
		return false;
	}	
	settings_set_imei(imei);
	
	modem_status = ModemSmsDeleteAllMessages(25000UL);
	ESP_LOGI(pcTaskGetName(NULL), "Delete all SMS messages %s", ModemStatusToText(modem_status));	
	if (modem_status != MODEM_OK)
	{
		return false;
	}			
		
	modem_status = ModemSetManualDataRead(250UL);
	ESP_LOGI(pcTaskGetName(NULL), "Set manual read %s", ModemStatusToText(modem_status));	
	if (modem_status != MODEM_OK)
	{
		return false;
	}	
	
	modem_status = ModemSetSmsPduMode(250UL);
	ESP_LOGI(pcTaskGetName(NULL), "Set SMS text mode %s", ModemStatusToText(modem_status));	
	if (modem_status != MODEM_OK)
	{
		return false;
	}	
	
	modem_status = ModemSetSmsReceiveMode(250UL);
	ESP_LOGI(pcTaskGetName(NULL), "Set SMS receive mode %s", ModemStatusToText(modem_status));	
	if (modem_status != MODEM_OK)
	{
		return false;
	}			
	
	return true;
}

static bool modem_activate_data_connection(void)
{
	ModemStatus_t modem_status;
	char ipAddress[MODEM_MAX_IP_ADDRESS_LENGTH + 1] = "";
	
	modem_status = ModemDeactivateDataConnection(40000UL);
	ESP_LOGI(pcTaskGetName(NULL), "Deactivate data connection %s", ModemStatusToText(modem_status));	
	if (modem_status != MODEM_SHUT_OK)
	{
		return false;
	}	
	
	modem_status = ModemConfigureDataConnection(settings_get_apn(), settings_get_apn_user_name(), settings_get_apn_password(), 250UL);
	ESP_LOGI(pcTaskGetName(NULL), "Configure data connection %s", ModemStatusToText(modem_status));	
	if (modem_status != MODEM_OK)
	{
		return false;
	}			

	modem_status = ModemActivateDataConnection(40000UL);
	ESP_LOGI(pcTaskGetName(NULL), "Activate data connection %s", ModemStatusToText(modem_status));	
	if (modem_status != MODEM_OK)
	{
		return false;
	}		

	modem_status = ModemGetOwnIpAddress(ipAddress, MODEM_MAX_IP_ADDRESS_LENGTH + 1, 250UL);
	ESP_LOGI(pcTaskGetName(NULL), "Get own IP address %s %s", ModemStatusToText(modem_status), ipAddress);	
	if (modem_status != MODEM_OK)
	{
		return false;
	}	

	return true;	
}

static bool modem_open_data_connection(void)
{
	ModemStatus_t modem_status;
	MqttStatus_t mqtt_status;
	
	modem_status = ModemOpenTcpConnection(settings_get_mqtt_broker_address(), settings_get_mqtt_broker_port(), 8000UL);
	
	ESP_LOGI(pcTaskGetName(NULL), "Open TCP connection %s", ModemStatusToText(modem_status));	
	if (modem_status != MODEM_OK)
	{
		return false;
	}
	
	mqtt_status = MqttConnect("1234", NULL, NULL, 600U, 20000UL);
	ESP_LOGI(pcTaskGetName(NULL), "MQTT connect %s", MqttStatusToText(mqtt_status));	
	
	if (mqtt_status != MQTT_OK)
	{
		return false;
	}	
	
	return true;
}

static bool modem_start(void)
{
	if (!modem_network_register())
	{
		return false;
	}

	if (!modem_set_parameters())
	{
		return false;
	}
	
	return true;
}

void boat_iot_task(void *parameters)
{
	ModemStatus_t modem_status;
	MqttStatus_t mqtt_status;
	uint8_t strength;	
	bool modem_start_success;
	uint8_t failed_loop_count = 0U;
	bool loop_failed;
	char buf[20];	
	uint32_t sms_id;
	uint32_t i;
	uint16_t properties_parsed;

float speed_over_ground_data = 0.0f;		// todo
float heading_data = 0.0f;		// todo
float boat_speed_data = 0.0f;		// todo
	
	ESP_LOGI(pcTaskGetName(NULL), "Boat iot task started");
	
	// signal main task that this task has started
	(void)xTaskNotifyGive(get_main_task_handle());
	
	do
	{
		(void)ModemInit();
		modem_start_success = modem_start();	
		if (!modem_start_success)
		{
			ESP_LOGI(pcTaskGetName(NULL), "Failed to start modem");			
			ModemDelete();
		}
		
		vTaskDelay(2000UL);													
	}
	while (!modem_start_success);
	
	while(true)
	{	
		if (settings_get_boat_iot_started())
		{
			loop_failed = false;
			if (!ModemGetPdpActivatedState())
			{
				loop_failed = !modem_activate_data_connection();
			}
			
			if (!loop_failed && !ModemGetTcpConnectedState())
			{
				loop_failed = !modem_open_data_connection();
			}
			
			if (!loop_failed)
			{		
				if ((mqtt_status = MqttHandleResponse(5000UL)) != MQTT_NO_RESPONSE)
				{
					ESP_LOGI(pcTaskGetName(NULL), "Handle response %s", MqttStatusToText(mqtt_status));		

					if (mqtt_status	< MQTT_OK)
					{
						loop_failed = true;
					}	
				}
			}
			
			if (!loop_failed)
			{			
				modem_status = ModemGetSignalStrength(&strength, 250UL);
				ESP_LOGI(pcTaskGetName(NULL), "Signal strength %s %u", ModemStatusToText(modem_status), (uint32_t)strength);	
				
				if (modem_status != MODEM_OK)
				{
					loop_failed = true;
				}	
			}		

			if (!loop_failed)
			{	
				if (ModemGetTcpConnectedState())
				{
					snprintf(buf, sizeof(buf), "%u", (uint32_t)strength);
					mqtt_status = MqttPublish(MQTT_PUBLISH_TOPIC_ROOT "/strength", (uint8_t *)buf, strlen(buf), false, 5000UL);	
					ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish strength %u %s", (uint32_t)strength, MqttStatusToText(mqtt_status));	

					if (mqtt_status != MQTT_OK)
					{
						loop_failed = true;
					}					
				}			
			}	
			
			if (!loop_failed)
			{			
	speed_over_ground_data += 0.1;
				if (ModemGetTcpConnectedState())
				{
					snprintf(buf, sizeof(buf), "%.1f", speed_over_ground_data);
					mqtt_status = MqttPublish(MQTT_PUBLISH_TOPIC_ROOT "/sog", (uint8_t *)buf, strlen(buf), false, 5000UL);	
					ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish sog %f %s", speed_over_ground_data, MqttStatusToText(mqtt_status));		
					
					if (mqtt_status != MQTT_OK)
					{
						loop_failed = true;
					}	
				}				
			}		
			
			if (!loop_failed)
			{			
	boat_speed_data += 0.12;
				if (ModemGetTcpConnectedState())
				{		
					snprintf(buf, sizeof(buf), "%.1f", boat_speed_data);		
					mqtt_status = MqttPublish(MQTT_PUBLISH_TOPIC_ROOT "/boatspeed", (uint8_t *)buf, strlen(buf), false, 5000UL);	
					ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish boatspeed %f %s", boat_speed_data, MqttStatusToText(mqtt_status));	
					
					if (mqtt_status != MQTT_OK)
					{
						loop_failed = true;
					}				
				}			
			}		
					
			if (!loop_failed)
			{	
	heading_data +=7.0f;
	if (heading_data >= 360.0f) heading_data = 0.0f;	
				if (ModemGetTcpConnectedState())
				{		
					snprintf(buf, sizeof(buf), "%.1f", heading_data);
					mqtt_status = MqttPublish(MQTT_PUBLISH_TOPIC_ROOT "/heading", (uint8_t *)buf, strlen(buf), false, 5000UL);	
					ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish heading %f %s", heading_data, MqttStatusToText(mqtt_status));	
					
					if (mqtt_status != MQTT_OK)
					{
						loop_failed = true;
					}				
				}			
			}	
			
			if (loop_failed)
			{
				failed_loop_count++;
			}
			else
			{
				failed_loop_count = 0U;
			}
			
			if (failed_loop_count == 10U)
			{
				failed_loop_count = 0U;
				ESP_LOGI(pcTaskGetName(NULL), "************ REBOOTING ************");			
				esp_restart();	
			}			
		}
		
		for (i = 0UL; i < settings_get_period_s(); i++)
		{
			if (new_sms_check(&sms_id))
			{
				char phone_number[SMS_MAX_PHONE_NUMBER_LENGTH + 1];
				char message_text[MODEM_SMS_MAX_TEXT_LENGTH + 1];
				
				properties_parsed = 0U;
				if (sms_receive(sms_id, phone_number, SMS_MAX_PHONE_NUMBER_LENGTH + 1, message_text, MODEM_SMS_MAX_TEXT_LENGTH + 1))
				{
					ESP_LOGI(pcTaskGetName(NULL), "SMS text %s", message_text);						
					settings_set_phone_number(phone_number);					
					properties_parsed = property_parse(message_text, config_parser_callback);
					ESP_LOGI(pcTaskGetName(NULL), "%u properties parsed", properties_parsed);						
				}
				modem_status = ModemSmsDeleteAllMessages(25000UL);
				ESP_LOGI(pcTaskGetName(NULL), "Delete all SMS messages %s", ModemStatusToText(modem_status));	

				if (settings_get_restart_needed())
				{
					esp_restart();
				}
			}
			vTaskDelay(1000UL);											
		}		
	}
}

static bool config_parser_callback(char *key, char *value)
{
	bool found = false;
	char message_text[MODEM_SMS_MAX_TEXT_LENGTH + 1];
	uint32_t period;
	
	if (!key || !value)
	{
		return false;
	}
	
	capitalize_string(key);
	
	if (strcmp(key, "APN") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Property apn=%s", value);	
		settings_set_apn(value);
		settings_save();
		settings_set_restart_needed(true);
		found = true;
	}
	else if (strcmp(key, "USER") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Property user=%s", value);	
		settings_set_apn_user_name(value);
		settings_save();
		settings_set_restart_needed(true);		
		found = true;
	}
	else if (strcmp(key, "PASS") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Property password=%s", value);	
		settings_set_apn_password(value);
		settings_save();
		settings_set_restart_needed(true);
		found = true;
	}	
	else if (strcmp(key, "BROKER") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Property broker=%s", value);	
		settings_set_mqtt_broker_address(value);
		settings_save();
		settings_set_restart_needed(true);
		found = true;
	}	
	else if (strcmp(key, "PORT") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Property port=%s", value);	
		settings_set_mqtt_broker_port(atoi(value));
		settings_save();
		settings_set_restart_needed(false);
		found = true;
	}	
	else if (strcmp(key, "PERIOD") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Property period=%s", value);	
		if (hms_to_seconds(value, &period))
		{
			if (period >= 5UL)
			{
				settings_set_period_s(period);		
				settings_save();
			}
		}
		found = true;
	}		
	else if (strcmp(key, "SETTINGS") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Command settings");	
		
		char started_stopped_buf[8];
		
		if (settings_get_boat_iot_started())
		{
			strcpy(started_stopped_buf, "Started");
		}
		else
		{
			strcpy(started_stopped_buf, "Stopped");
		}
		
		snprintf(message_text, (size_t)MODEM_SMS_MAX_TEXT_LENGTH + 1, "APN=%s\nUser=%s\nPass=%s\nBroker=%s\nPort=%u\nPeriod=%s\n%s",
			settings_get_apn(), settings_get_apn_user_name(), settings_get_apn_password(),
			settings_get_mqtt_broker_address(), (uint32_t)settings_get_mqtt_broker_port(),
			seconds_to_hms(settings_get_period_s()), started_stopped_buf);
		(void)sms_send(message_text, settings_get_phone_number());		
		found = true;			
	}
	else if (strcmp(key, "IMEI") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Command imei");	
		
		snprintf(message_text, (size_t)MODEM_SMS_MAX_TEXT_LENGTH + 1, "IMEI:%s", settings_get_imei());
		(void)sms_send(message_text, settings_get_phone_number());		
		found = true;
	}	
	else if (strcmp(key, "START") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Command start");	
		
		settings_set_boat_iot_started(true);
		(void)sms_send("Started", settings_get_phone_number());				
		found = true;					
	}	
	else if (strcmp(key, "STOP") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Command stop");	
		
		settings_set_boat_iot_started(false);
		(void)sms_send("Stopped", settings_get_phone_number());				
		found = true;		
	}	
	else if (strcmp(key, "RESTART") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Command restart");	
		
		settings_set_restart_needed(true);
		(void)sms_send("Restarting", settings_get_phone_number());				
		found = true;		
	}		
	
	return found;
}
