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
#include "blue_thing.h"
#include "timer.h"

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
	settings_set_code(util_hash_djb2(imei));
	
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
	uint32_t sms_id;
	uint32_t i;
	uint16_t properties_parsed;
	uint32_t time_ms;
	char mqtt_topic[20];
	char mqtt_data_buf[200];	
	char number_buf[20];
	
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
/*
			if (!loop_failed && ModemGetTcpConnectedState())
			{
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%u", (uint32_t)strength);
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/strength", settings_get_code());			
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);	
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish strength %u %s", (uint32_t)strength, MqttStatusToText(mqtt_status));	

				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
				}					
			}						

			// publish depth
			time_ms = timer_get_time_ms();		
			if (!loop_failed && 
					ModemGetTcpConnectedState() && 
					(time_ms - boat_data_reception_time.depth_received_time < DEPTH_MAX_DATA_AGE_MS || boat_data_reception_time.depth_received_time > time_ms))
			{			
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%.1f", depth_data);
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/depth", settings_get_code());			
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);	
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish depth %f %s", depth_data, MqttStatusToText(mqtt_status));		
				
				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
				}	
			}					
			
			// publish heading
			time_ms = timer_get_time_ms();		
			if (!loop_failed && 
					ModemGetTcpConnectedState() && 
					(time_ms - boat_data_reception_time.heading_true_received_time < HEADING_TRUE_MAX_DATA_AGE_MS || boat_data_reception_time.heading_true_received_time > time_ms))
			{
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%u", (unsigned int)heading_true_data);
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/heading", settings_get_code());			
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);	
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish heading %f %s", heading_true_data, MqttStatusToText(mqtt_status));		
				
				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
				}	
			}		

			// publish trip
			time_ms = timer_get_time_ms();			
			if (!loop_failed && 
					ModemGetTcpConnectedState() && 
					(time_ms - boat_data_reception_time.trip_received_time < TRIP_MAX_DATA_AGE_MS || boat_data_reception_time.trip_received_time > time_ms))
			{			
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%.1f", trip_data);
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/trip", settings_get_code());			
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);	
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish trip %f %s", trip_data, MqttStatusToText(mqtt_status));		
				
				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
				}	
			}		

			// publish total distance			
			time_ms = timer_get_time_ms();			
			if (!loop_failed && 
					ModemGetTcpConnectedState() && 
					(time_ms - boat_data_reception_time.total_distance_received_time < TOTAL_DISTANCE_MAX_DATA_AGE_MS || boat_data_reception_time.total_distance_received_time > time_ms))
			{			
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%u", (unsigned int)total_distance_data);
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/log", settings_get_code());			
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);	
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish log %u %s", (unsigned int)total_distance_data, MqttStatusToText(mqtt_status));		
				
				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
				}	
			}				
			
			// publish boat speed			
			time_ms = timer_get_time_ms();			
			if (!loop_failed && 
					ModemGetTcpConnectedState() && 
					(time_ms - boat_data_reception_time.boat_speed_received_time < BOAT_SPEED_MAX_DATA_AGE_MS || boat_data_reception_time.boat_speed_received_time > time_ms))
			{			
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%.1f", boat_speed_data);
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/boatspeed", settings_get_code());			
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);	
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish boatspeed %f %s", boat_speed_data, MqttStatusToText(mqtt_status));	
				
				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
				}	
			}		

			// publish sog		
			time_ms = timer_get_time_ms();			
			if (!loop_failed && 
					ModemGetTcpConnectedState() && 
					(time_ms - boat_data_reception_time.speed_over_ground_received_time < SOG_MAX_DATA_AGE_MS || boat_data_reception_time.speed_over_ground_received_time > time_ms))
			{			
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%.1f", speed_over_ground_data);
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/sog", settings_get_code());			
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);	
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish sog %f %s", speed_over_ground_data, MqttStatusToText(mqtt_status));		
				
				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
				}	
			}		

			// publish temperature		
			time_ms = timer_get_time_ms();			
			if (!loop_failed && 
					ModemGetTcpConnectedState() && 
					(time_ms - boat_data_reception_time.seawater_temperature_received_time < TEMPERATURE_MAX_DATA_AGE_MS || boat_data_reception_time.seawater_temperature_received_time > time_ms))
			{			
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%.1f", seawater_temeperature_data);
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/temp", settings_get_code());			
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);	
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish temp %f %s", seawater_temeperature_data, MqttStatusToText(mqtt_status));		
				
				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
				}	
			}	

			// publish cog		
			time_ms = timer_get_time_ms();			
			if (!loop_failed && 
					ModemGetTcpConnectedState() && 
					(time_ms - boat_data_reception_time.course_over_ground_received_time < COG_MAX_DATA_AGE_MS || boat_data_reception_time.course_over_ground_received_time > time_ms))
			{			
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%hu", course_over_ground_data);
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/cog", settings_get_code());			
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);	
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish cog %hu %s", course_over_ground_data, MqttStatusToText(mqtt_status));		
				
				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
				}	
			}			
*/
			// publish all data in one
			if (!loop_failed && ModemGetTcpConnectedState())
			{				
				snprintf(mqtt_topic, sizeof(mqtt_topic), "%08X/all", settings_get_code());							
				time_ms = timer_get_time_ms();			
				
				snprintf(mqtt_data_buf, sizeof(mqtt_data_buf), "%hhu,", strength);
				
				if (time_ms - boat_data_reception_time.course_over_ground_received_time < COG_MAX_DATA_AGE_MS || boat_data_reception_time.course_over_ground_received_time > time_ms)
				{
					snprintf(number_buf, sizeof(number_buf), "%hu", course_over_ground_data);
					strcat(mqtt_data_buf, number_buf);
				}
				strcat(mqtt_data_buf, ",");
				
				if (time_ms - boat_data_reception_time.seawater_temperature_received_time < TEMPERATURE_MAX_DATA_AGE_MS || boat_data_reception_time.seawater_temperature_received_time > time_ms)
				{
					snprintf(number_buf, sizeof(number_buf), "%.1f", seawater_temeperature_data);
					strcat(mqtt_data_buf, number_buf);
				}
				strcat(mqtt_data_buf, ",");

				if (time_ms - boat_data_reception_time.speed_over_ground_received_time < SOG_MAX_DATA_AGE_MS || boat_data_reception_time.speed_over_ground_received_time > time_ms)
				{
					snprintf(number_buf, sizeof(number_buf), "%.1f", speed_over_ground_data);
					strcat(mqtt_data_buf, number_buf);
				}
				strcat(mqtt_data_buf, ",");
				
				if (time_ms - boat_data_reception_time.boat_speed_received_time < BOAT_SPEED_MAX_DATA_AGE_MS || boat_data_reception_time.boat_speed_received_time > time_ms)
				{
					snprintf(number_buf, sizeof(number_buf), "%.1f", boat_speed_data);
					strcat(mqtt_data_buf, number_buf);
				}
				strcat(mqtt_data_buf, ",");

				if (time_ms - boat_data_reception_time.total_distance_received_time < TOTAL_DISTANCE_MAX_DATA_AGE_MS || boat_data_reception_time.total_distance_received_time > time_ms)
				{
					snprintf(number_buf, sizeof(number_buf), "%u", (unsigned int)total_distance_data);
					strcat(mqtt_data_buf, number_buf);
				}
				strcat(mqtt_data_buf, ",");

				if (time_ms - boat_data_reception_time.trip_received_time < TRIP_MAX_DATA_AGE_MS || boat_data_reception_time.trip_received_time > time_ms)
				{
					snprintf(number_buf, sizeof(number_buf), "%.1f", trip_data);
					strcat(mqtt_data_buf, number_buf);
				}
				strcat(mqtt_data_buf, ",");

				if (time_ms - boat_data_reception_time.heading_true_received_time < HEADING_TRUE_MAX_DATA_AGE_MS || boat_data_reception_time.heading_true_received_time > time_ms)
				{
					snprintf(number_buf, sizeof(number_buf), "%u", (unsigned int)heading_true_data);
					strcat(mqtt_data_buf, number_buf);
				}
				strcat(mqtt_data_buf, ",");

				if (time_ms - boat_data_reception_time.depth_received_time < DEPTH_MAX_DATA_AGE_MS || boat_data_reception_time.depth_received_time > time_ms)
				{
					snprintf(number_buf, sizeof(number_buf), "%.1f", depth_data);
					strcat(mqtt_data_buf, number_buf);
				}
				strcat(mqtt_data_buf, ",");
				
float dummy = 0.1f;								
				
				snprintf(number_buf, sizeof(number_buf), "%.1f", dummy); // tws
				strcat(mqtt_data_buf, number_buf);
				strcat(mqtt_data_buf, ",");

				snprintf(number_buf, sizeof(number_buf), "%.1f", dummy); // twa
				strcat(mqtt_data_buf, number_buf);
				strcat(mqtt_data_buf, ",");

				snprintf(number_buf, sizeof(number_buf), "%.1f", dummy); // aws
				strcat(mqtt_data_buf, number_buf);
				strcat(mqtt_data_buf, ",");

				snprintf(number_buf, sizeof(number_buf), "%.1f", dummy); // awa
				strcat(mqtt_data_buf, number_buf);
				strcat(mqtt_data_buf, ",");

				snprintf(number_buf, sizeof(number_buf), "%.1f", dummy); // lat
				strcat(mqtt_data_buf, number_buf);
				strcat(mqtt_data_buf, ",");

				snprintf(number_buf, sizeof(number_buf), "%.1f", dummy); // long
				strcat(mqtt_data_buf, number_buf);
			
				
				
				mqtt_status = MqttPublish(mqtt_topic, (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);					
				//mqtt_status = MqttPublish("BluePillDemo/sog", (uint8_t *)mqtt_data_buf, strlen(mqtt_data_buf), false, 5000UL);					
				
				ESP_LOGI(pcTaskGetName(NULL), "Mqtt publish %s %s %s", mqtt_topic, mqtt_data_buf, MqttStatusToText(mqtt_status));		
				
				if (mqtt_status != MQTT_OK)
				{
					loop_failed = true;
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
			if (sms_check_for_new(&sms_id))
			{
				char phone_number[SMS_MAX_PHONE_NUMBER_LENGTH + 1];
				char message_text[MODEM_SMS_MAX_TEXT_LENGTH + 1];
				
				properties_parsed = 0U;
				if (sms_receive(sms_id, phone_number, SMS_MAX_PHONE_NUMBER_LENGTH + 1, message_text, MODEM_SMS_MAX_TEXT_LENGTH + 1))
				{
					ESP_LOGI(pcTaskGetName(NULL), "SMS text %s", message_text);						
					settings_set_phone_number(phone_number);					
					properties_parsed = property_parse(message_text, config_parser_callback);
					ESP_LOGI(pcTaskGetName(NULL), "%u settings parsed", properties_parsed);						
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
	
	util_capitalize_string(key);
	
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
		if (util_hms_to_seconds(value, &period))
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
			util_seconds_to_hms(settings_get_period_s()), started_stopped_buf);
		(void)sms_send(message_text, settings_get_phone_number());		
		found = true;			
	}
	else if (strcmp(key, "CODE") == 0)
	{
		ESP_LOGI(pcTaskGetName(NULL), "Command code");	
		
		snprintf(message_text, (size_t)MODEM_SMS_MAX_TEXT_LENGTH + 1, "Code=%08X", settings_get_code());
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
