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

#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/***************
*** INCLUDES ***
***************/

#include <stdint.h>

/**************
*** DEFINES ***
**************/

#define SETTINGS_MQTT_BROKER_ADDRESS_MAX_LENGTH			32UL			///< Maximum length in bytes of MQTT broker's IP address

/************
*** TYPES ***
************/

/*************************
*** EXTERNAL VARIABLES ***
*************************/

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Initialize the settings driver. If settings not previosuly saved this sets non-volatile settings to defaults and saves them.
 * If previously saved this loads the non volatile settings. In both cases volatile settings are set to defaults. Call once
 * at startup before using other functions.
 * 
 * @note Subsequent calls are ignored
 */
void settings_init(void);

/**
 * Reset all settings to defaults
 */
void settings_reset(void);

/**
 * Serialize non-volatile settings to flash 
 */
void settings_save(void);

/**
 * Read device address non-volatile setting from memory copy
 *
 * @return The setting's value
 */
uint8_t settings_get_device_address(void);

/**
 * Save device address non-volatile setting in memory copy.
 *
 * @param  New value of the setting
 * @note This does not save the new setting in flash memory
 */
void settings_set_device_address(uint8_t device_address);

/**
 * Read GSM operator APN non-volatile setting from memory copy
 *
 * @return The setting's value
 */
 const char *settings_get_apn(void);

/**
 * Save GSM operator APN non-volatile setting in memory copy.
 *
 * @param  New value of the setting
 * @note This does not save the new setting in flash memory
 */
void settings_set_apn(const char *apn);

/**
 * Read GSM operator user name non-volatile setting from memory copy
 *
 * @return The setting's value
 */
const char *settings_get_apn_user_name(void);

/**
 * Save GSM operator user name non-volatile setting in memory copy.
 *
 * @param  New value of the setting
 * @note This does not save the new setting in flash memory
 */
void settings_set_apn_user_name(const char *apn_user_name);

/**
 * Read GSM operator password non-volatile setting from memory copy
 *
 * @return The setting's value
 */
const char *settings_get_apn_password(void);

/**
 * Save GSM operator password non-volatile setting in memory copy.
 *
 * @param  New value of the setting
 * @note This does not save the new setting in flash memory
 */
void settings_set_apn_password(const char *apn_password);

/**
 * Read MQTT broker IP address non-volatile setting from memory copy
 *
 * @return The setting's value
 */
const char *settings_get_mqtt_broker_address(void);

/**
 * Save MQTT broker IP address non-volatile setting in memory copy.
 *
 * @param  New value of the setting
 * @note This does not save the new setting in flash memory
 */
void settings_set_mqtt_broker_address(const char *mqtt_broker_address);

/**
 * Read MQTT broker port non-volatile setting from memory copy
 *
 * @return The setting's value
 */
uint16_t settings_get_mqtt_broker_port(void);

/**
 * Save MQTT broker port non-volatile setting in memory copy.
 *
 * @param  New value of the setting
 * @note This does not save the new setting in flash memory
 */
void settings_set_mqtt_broker_port(uint16_t mqtt_broker_port);

/**
 * Read hashed IMEI volatile setting from memory
 *
 * @return The setting's value
 */
uint32_t settings_get_hashed_imei(void);

/**
 * Save hashed IMEI volatile setting in memory.
 *
 * @param  New value of the setting
 * @note This settings is never saved in flash memory
 */
 void settings_set_hashed_imei(uint32_t code);

/**
 * Read SMS sender's phone number volatile setting from memory
 *
 * @return The setting's value
 */
 const char *settings_get_phone_number(void);

/**
 * Save SMS sender's phone number volatile setting in memory.
 *
 * @param  New value of the setting
 * @note This settings is never saved in flash memory
 */
void settings_set_phone_number(const char *phone_number);

/**
 * Read if MQTT publishing has been started volatile setting from memory
 *
 * @return The setting's value
 */
bool settings_get_publishing_started(void);

/**
 * Save if MQTT publishing has been started volatile setting in memory.
 *
 * @param  New value of the setting
 * @note This settings is never saved in flash memory
 */
void settings_set_publishing_started(bool boat_iot_started);

/**
 * Read if a device reboot is needed volatile setting from memory
 *
 * @return The setting's value
 */
bool settings_get_reboot_needed(void);

/**
 * Save if a device reboot is needed volatile setting in memory.
 *
 * @param  New value of the setting
 * @note This settings is never saved in flash memory
 */
void settings_set_reboot_needed(bool restart_needed);

/**
 * Read MQTT publishing period non-volatile setting from memory copy
 *
 * @return The setting's value
 */
uint32_t settings_get_publishing_period_s(void);

/**
 * Save MQTT publishing period non-volatile setting in memory copy.
 *
 * @param  New value of the setting
 * @note This does not save the new setting in flash memory
 */
void settings_set_publishing_period_s(uint32_t period_s);

/**
 * Read if a publisghing start is needed volatile setting from memory
 *
 * @return The setting's value
 */
bool settings_get_publishing_start_needed(void);

/**
 * Save if a publisghing start is needed volatile setting in memory.
 *
 * @param  New value of the setting
 * @note This settings is never saved in flash memory
 */
void settings_set_publishing_start_needed(bool publishing_start_needed);

#ifdef __cplusplus
}
#endif

#endif   // SMS_SMS_H_