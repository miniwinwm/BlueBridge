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

#define FAKE_DATA

#define NETWORK_REGISTRATION_WAIT_TIME_MS		60000UL					// time to wait for network registration before giving up
#define PRESSURE_MAX_DATA_AGE_MS				30000UL
#define GMT_MAX_DATA_AGE_MS						12000UL
#define DATE_MAX_DATA_AGE_MS					12000UL
#define COG_MAX_DATA_AGE_MS						4000UL
#define SOG_MAX_DATA_AGE_MS						4000UL
#define LATITUDE_MAX_DATA_AGE_MS				4000UL
#define LONGITUDE_MAX_DATA_AGE_MS				4000UL
#define DEPTH_MAX_DATA_AGE_MS					4000UL
#define HEADING_TRUE_MAX_DATA_AGE_MS			4000UL
#define BOAT_SPEED_MAX_DATA_AGE_MS				4000UL
#define WMM_CALCULATION_MAX_DATA_AGE			(60UL * 60UL * 1000UL)
#define APPARENT_WIND_ANGLE_MAX_DATA_AGE_MS		4000UL
#define APPARENT_WIND_SPEED_MAX_DATA_AGE_MS		4000UL
#define TRIP_MAX_DATA_AGE_MS					8000UL
#define TOTAL_DISTANCE_MAX_DATA_AGE_MS			8000UL
#define TEMPERATURE_MAX_DATA_AGE_MS				4000UL
#define TRUE_WIND_ANGLE_MAX_DATA_AGE_MS			4000UL
#define TRUE_WIND_SPEED_MAX_DATA_AGE_MS			4000UL
#define WIND_DIRECTION_MAGNETIC_MAX_DATA_AGE_MS	4000UL
#define WIND_DIRECTION_TRUE_MAX_DATA_AGE_MS		4000UL

/************
*** TYPES ***
************/

typedef struct
{
	uint8_t year;
	uint8_t month;
	uint8_t date;
} my_date_t;

typedef struct
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} my_time_t;

typedef struct
{
	uint32_t pressure_received_time;
	uint32_t speed_over_ground_received_time;	
	uint32_t course_over_ground_received_time;
	uint32_t latitude_received_time;
	uint32_t longitude_received_time;
	uint32_t gmt_received_time;
	uint32_t date_received_time;
	uint32_t wmm_calculation_time;	
	uint32_t depth_received_time;
	uint32_t heading_true_received_time;
	uint32_t boat_speed_received_time;
	uint32_t apparent_wind_speed_received_time;
	uint32_t apparent_wind_angle_received_time;	
	uint32_t true_wind_speed_received_time;
	uint32_t true_wind_angle_received_time;		
	uint32_t trip_received_time;
	uint32_t total_distance_received_time;	
	uint32_t seawater_temperature_received_time;
	uint32_t wind_direction_magnetic_received_time;
	uint32_t wind_direction_true_received_time;
} boat_data_reception_time_t;

/*************************
*** EXTERNAL VARIABLES ***
*************************/

extern volatile float variation_wmm_data;
extern volatile float pressure_data;
extern volatile float speed_over_ground_data;
extern volatile float latitude_data;
extern volatile float longitude_data;
extern volatile int16_t course_over_ground_data;
extern volatile float depth_data;
extern volatile float heading_true_data;
extern volatile float boat_speed_data;
extern volatile float apparent_wind_speed_data;
extern volatile float apparent_wind_angle_data;
extern volatile float true_wind_speed_data;
extern volatile float true_wind_angle_data;
extern volatile float trip_data;
extern volatile float total_distance_data;
extern volatile float seawater_temeperature_data;
extern volatile float wind_direction_magnetic_data;
extern volatile float wind_direction_true_data;
extern volatile my_time_t gmt_data;
extern volatile my_date_t date_data;
extern volatile boat_data_reception_time_t boat_data_reception_time;

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

TaskHandle_t get_main_task_handle(void);

#ifdef __cplusplus
}
#endif

#endif
