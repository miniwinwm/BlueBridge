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

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

/***************
*** INCLUDES ***
***************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**************
*** DEFINES ***
**************/

#define CREATE_TEST_DATA_CODE												///< If create test data code is included in build, comment out to remove
#define NETWORK_REGISTRATION_WAIT_TIME_MS		60000UL						///< Time to wait in millisecondsfor network registration before giving up
#define PRESSURE_MAX_DATA_AGE_MS				30000UL						///< Maximum age allowed for atmospheric pressure data before it is considered stale in milliseconds
#define GMT_MAX_DATA_AGE_MS						12000UL						///< Maximum age allowed for time data before it is considered stale in milliseconds
#define DATE_MAX_DATA_AGE_MS					12000UL						///< Maximum age allowed for date data before it is considered stale in milliseconds
#define COG_MAX_DATA_AGE_MS						4000UL						///< Maximum age allowed for COG data before it is considered stale in milliseconds
#define SOG_MAX_DATA_AGE_MS						4000UL						///< Maximum age allowed for SOG data before it is considered stale in milliseconds
#define LATITUDE_MAX_DATA_AGE_MS				4000UL						///< Maximum age allowed for latitude data before it is considered stale in milliseconds
#define LONGITUDE_MAX_DATA_AGE_MS				4000UL						///< Maximum age allowed for longitude data before it is considered stale in milliseconds
#define DEPTH_MAX_DATA_AGE_MS					4000UL						///< Maximum age allowed for depth data before it is considered stale in milliseconds
#define HEADING_TRUE_MAX_DATA_AGE_MS			4000UL						///< Maximum age allowed for compass heading data before it is considered stale in milliseconds
#define BOAT_SPEED_MAX_DATA_AGE_MS				4000UL						///< Maximum age allowed for boat speed through water data before it is considered stale in milliseconds
#define WMM_CALCULATION_MAX_DATA_AGE			(60UL * 60UL * 1000UL)		///< Maximum age allowed for world magnetic model calculation data before it is considered stale in milliseconds
#define APPARENT_WIND_ANGLE_MAX_DATA_AGE_MS		4000UL						///< Maximum age allowed for apparent wind angle data before it is considered stale in milliseconds
#define APPARENT_WIND_SPEED_MAX_DATA_AGE_MS		4000UL						///< Maximum age allowed for apparent wind speed data before it is considered stale in milliseconds
#define TRIP_MAX_DATA_AGE_MS					8000UL						///< Maximum age allowed for trip distance data before it is considered stale in milliseconds
#define TOTAL_DISTANCE_MAX_DATA_AGE_MS			8000UL						///< Maximum age allowed for total distance data before it is considered stale in milliseconds
#define TEMPERATURE_MAX_DATA_AGE_MS				4000UL						///< Maximum age allowed for water temperature data before it is considered stale in milliseconds
#define TRUE_WIND_ANGLE_MAX_DATA_AGE_MS			4000UL						///< Maximum age allowed for true wind angle data before it is considered stale in milliseconds
#define TRUE_WIND_SPEED_MAX_DATA_AGE_MS			4000UL						///< Maximum age allowed for truw wind speed data before it is considered stale in milliseconds
#define WIND_DIRECTION_MAGNETIC_MAX_DATA_AGE_MS	4000UL						///< Maximum age allowed for wind direction magnetic data before it is considered stale in milliseconds
#define WIND_DIRECTION_TRUE_MAX_DATA_AGE_MS		4000UL						///< Maximum age allowed for wind direction true data before it is considered stale in milliseconds

/************
*** TYPES ***
************/

/**
 * Structure to hold a date
 */
typedef struct
{
	uint8_t year;		///< Year of 21st century 0-99
	uint8_t month;		///< Month 1-12
	uint8_t date;		///< Date 1-31
} my_date_t;

/**
 * Structure to hold a time
 */
typedef struct
{
	uint8_t hour;		///< Hour 0-23
	uint8_t minute;		///< Minute 0-59
	uint8_t second;		///< Second 0-59
} my_time_t;

/**
 * Structure to hold the time data values were last received in milliseconds since system start
 */
typedef struct
{
	uint32_t pressure_received_time;					///< atmospheric pressure last received time
	uint32_t speed_over_ground_received_time;			///< SOG last received time
	uint32_t course_over_ground_received_time;			///< COG last received time
	uint32_t latitude_received_time;					///< latitude last received time
	uint32_t longitude_received_time;					///< longitude last received time
	uint32_t gmt_received_time;							///< time last received time
	uint32_t date_received_time;						///< date last received time
	uint32_t wmm_calculation_time;						///< WMM calculation last performed time
	uint32_t depth_received_time;						///< depth last received time
	uint32_t heading_true_received_time;				///< compass heading last received time
	uint32_t boat_speed_received_time;					///< boat speed last received time
	uint32_t apparent_wind_speed_received_time;			///< AWS last received time
	uint32_t apparent_wind_angle_received_time;			///< AWA last received time
	uint32_t true_wind_speed_received_time;				///< TWS last received time
	uint32_t true_wind_angle_received_time;				///< TWA last received time
	uint32_t trip_received_time;						///< trip distance last received time
	uint32_t total_distance_received_time;				///< total distance last received time
	uint32_t seawater_temperature_received_time;		///< water temperature last received time
	uint32_t wind_direction_magnetic_received_time;		///< wind direction magnetic last received time
	uint32_t wind_direction_true_received_time;			///< wind direction true last received time
} boat_data_reception_time_t;

/*************************
*** EXTERNAL VARIABLES ***
*************************/

extern volatile float variation_wmm_data;				///< Latest value of world magnetic model variation calculated data
extern volatile float pressure_data;					///< Latest value of atmospheric pressure data
extern volatile float speed_over_ground_data;			///< Latest value of SOG data
extern volatile float latitude_data;					///< Latest value of latitude data
extern volatile float longitude_data;					///< Latest value of longitude data
extern volatile int16_t course_over_ground_data;		///< Latest value of COG data
extern volatile float depth_data;						///< Latest value of depth data
extern volatile float heading_true_data;				///< Latest value of compass heading data
extern volatile float boat_speed_data;					///< Latest value of boat speed through water data
extern volatile float apparent_wind_speed_data;			///< Latest value of AWS data
extern volatile float apparent_wind_angle_data;			///< Latest value of AWA data
extern volatile float true_wind_speed_data;				///< Latest value of TWS data
extern volatile float true_wind_angle_data;				///< Latest value of TWA data
extern volatile float trip_data;						///< Latest value of trip distance data
extern volatile float total_distance_data;				///< Latest value of total distance data
extern volatile float seawater_temeperature_data;		///< Latest value of water temperature data
extern volatile float wind_direction_magnetic_data;		///< Latest value of wind direction magnetic data
extern volatile float wind_direction_true_data;			///< Latest value of wind direction true data
extern volatile my_time_t gmt_data;						///< Latest value of time data
extern volatile my_date_t date_data;					///< Latest value of date data
extern volatile boat_data_reception_time_t boat_data_reception_time;		///< struct that holds all boat data last received time

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Returns the task handle of the main task
 *
 * @return The task handle
 */
TaskHandle_t get_main_task_handle(void);

#ifdef __cplusplus
}
#endif

#endif
