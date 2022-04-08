#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "modem.h"

#define MQTT_BROKER_ADDRESS_MAX_LENGTH	32UL		// todo change name or move

void settings_init(void);
void settings_save(void);

uint8_t settings_get_device_address(void);
void settings_set_device_address(uint8_t device_address);

const char *settings_get_apn(void);
void settings_set_apn(const char *apn);

const char *settings_get_apn_user_name(void);
void settings_set_apn_user_name(const char *apn_user_name);

const char *settings_get_apn_password(void);
void settings_set_apn_password(const char *apn_password);

const char *settings_get_mqtt_broker_address(void);
void settings_set_mqtt_broker_address(const char *mqtt_broker_address);

uint16_t settings_get_mqtt_broker_port(void);
void settings_set_mqtt_broker_port(uint16_t mqtt_broker_port);

uint32_t settings_get_code(void);
void settings_set_code(uint32_t code);

const char *settings_get_phone_number(void);
void settings_set_phone_number(const char *phone_number);

bool settings_get_boat_iot_started(void);
void settings_set_boat_iot_started(bool boat_iot_started);

bool settings_get_restart_needed(void);
void settings_set_restart_needed(bool restart_needed);

uint32_t settings_get_period_s(void);
void settings_set_period_s(uint32_t period_s);

#ifdef __cplusplus
}
#endif

#endif   // SMS_SMS_H_