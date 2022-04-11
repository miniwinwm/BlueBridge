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

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "NMEA2000_CAN.h"  
#include "N2kMessages.h"
#include "pressure_sensor.h"
#include "main.h"
#include "esp_log.h"
#include "serial.h"
#include "nmea.h"
#include "timer.h"
#include "wmm.h"
#include "modem.h"
#include "mqtt.h"
#include "boat_iot.h"
#include "settings.h"
#include "sms.h"
#include "blue_thing.h"
#include "led.h"

#define PORT_N0183								0
#define PORT_BLUETOOTH							1
#define PRESSURE_SENSOR_TASK_STACK_SIZE			8096U
#define BOAT_IOT_TASK_STACK_SIZE				8096U
#define MAIN_TASK_SW_TIMER_COUNT				3
#define SW_TIMER_25_MS							0
#define SW_TIMER_1_S							1
#define SW_TIMER_8_S							2

typedef struct 
{
	unsigned int PGN;
	void (*Handler)(const tN2kMsg &N2kMsg); 
} tNMEA2000Handler;

static void RMC_receive_callback(char *data);
static void VDM_receive_callback(char *data);
static void GGA_receive_callback(char *data);
static void RMC_transmit_callback(void);
static void XDR_transmit_callback(void);
static void MDA_transmit_callback(void);
static void VDM_transmit_callback(void);
static void GGA_transmit_callback(void);
static void DPT_transmit_callback(void);
static void MTW_transmit_callback(void);
static void VHW_transmit_callback(void);
static void HDT_transmit_callback(void);
static void HDM_transmit_callback(void);
static void VLW_transmit_callback(void);
static void MWV_transmit_callback(void);
static void MWD_transmit_callback(void);
static void vTimerCallback25ms(TimerHandle_t xTimer);
static void vTimerCallback1s(TimerHandle_t xTimer);
static void vTimerCallback8s(TimerHandle_t xTimer);
static void depth_handler(const tN2kMsg &N2kMsg);
static void heading_handler(const tN2kMsg &N2kMsg);
static void boat_speed_handler(const tN2kMsg &N2kMsg);
static void wind_handler(const tN2kMsg &N2kMsg);
static void log_handler(const tN2kMsg &N2kMsg);
static void environmental_handler(const tN2kMsg &N2kMsg);
static void HandleNMEA2000Msg(const tN2kMsg &N2kMsg);
#ifdef FAKE_DATA
static void fake_data(void);
#endif

static tNMEA2000Handler NMEA2000Handlers[] =
{
	{128267UL, &depth_handler},
	{127250UL, &heading_handler},
	{128259UL, &boat_speed_handler},	
	{130306UL, &wind_handler},
	{128275UL, &log_handler},
	{130310UL, &environmental_handler}
};

static const unsigned long n2k_transmit_messages[] = {130310UL, // atmospheric pressure
													  0UL};
static const unsigned long n2k_receive_messages[] = {127250UL, 	// heading
													 128259UL, 	// boat speed
													 128267UL, 	// depth
													 130306UL,	// wind
													 128275UL,	// log
													 130310UL,	// environmental
													 0UL};

static StaticQueue_t pressure_sensor_queue;
static uint8_t pressure_sensor_queue_buffer[sizeof(float)];
static QueueHandle_t pressure_sensor_queue_handle;
static TimerHandle_t xTimers[MAIN_TASK_SW_TIMER_COUNT];
static TaskHandle_t main_task_handle;
static nmea_message_data_XDR_t nmea_message_data_XDR;
static nmea_message_data_MDA_t nmea_message_data_MDA;
static nmea_message_data_RMC_t nmea_message_data_RMC;
static nmea_message_data_VDM_t nmea_message_data_VDM;
static nmea_message_data_GGA_t nmea_message_data_GGA;
static nmea_message_data_DPT_t nmea_message_data_DPT;
static nmea_message_data_MTW_t nmea_message_data_MTW;
static nmea_message_data_VHW_t nmea_message_data_VHW;
static nmea_message_data_HDM_t nmea_message_data_HDM;
static nmea_message_data_HDT_t nmea_message_data_HDT;
static nmea_message_data_VLW_t nmea_message_data_VLW;
static nmea_message_data_MWV_t nmea_message_data_MWV;
static nmea_message_data_MWD_t nmea_message_data_MWD;
volatile float variation_wmm_data;
volatile float pressure_data;
volatile float speed_over_ground_data;
volatile float latitude_data;
volatile float longitude_data;
volatile int16_t course_over_ground_data;
volatile float depth_data;
volatile float heading_true_data;
volatile float boat_speed_data;
volatile float apparent_wind_speed_data;
volatile float apparent_wind_angle_data;
volatile float true_wind_speed_data;
volatile float true_wind_angle_data;
volatile float trip_data;
volatile float total_distance_data;
volatile float seawater_temeperature_data;
volatile float wind_direction_magnetic_data;
volatile float wind_direction_true_data;
volatile my_time_t gmt_data;
volatile my_date_t date_data;
volatile boat_data_reception_time_t boat_data_reception_time;

/* MWD transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_MWD = {nmea_message_MWD,	PORT_BLUETOOTH, 2000UL, MWD_transmit_callback, &nmea_message_data_MWD, (nmea_encoder_function_t)nmea_encode_MWD};

static void MWD_transmit_callback(void)
{
	nmea_message_data_MWD.wind_speed_knots = true_wind_speed_data;
	nmea_message_data_MWD.data_available = NMEA_MWD_WIND_SPEED_KTS_PRESENT;

	if (timer_get_time_ms() - boat_data_reception_time.wind_direction_magnetic_received_time < WIND_DIRECTION_MAGNETIC_MAX_DATA_AGE_MS)
	{
		nmea_message_data_MWD.data_available |= NMEA_MWD_WIND_DIRECTION_MAG_PRESENT;
		nmea_message_data_MWD.wind_direction_magnetic = wind_direction_magnetic_data;
	}
	
	if (timer_get_time_ms() - boat_data_reception_time.wind_direction_true_received_time < WIND_DIRECTION_TRUE_MAX_DATA_AGE_MS)
	{
		nmea_message_data_MWD.data_available |= NMEA_MWD_WIND_DIRECTION_TRUE_PRESENT;
		nmea_message_data_MWD.wind_direction_true = wind_direction_true_data;
	}
}

/* MWV transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_MWV = {nmea_message_MWV, PORT_BLUETOOTH, 1000UL, MWV_transmit_callback, &nmea_message_data_MWV, (nmea_encoder_function_t)nmea_encode_MWV};

static void MWV_transmit_callback(void)
{
	uint32_t time_ms = timer_get_time_ms();
	static bool message_type_toggle = false;

	nmea_message_data_MWV.data_available = NMEA_MWV_REFERENCE_PRESENT | NMEA_MWV_WIND_SPEED_UNITS_PRESENT;
	nmea_message_data_MWV.wind_speed_units = 'N';

	if (message_type_toggle)
	{
		nmea_message_data_MWV.reference = 'T';

		if (time_ms - boat_data_reception_time.true_wind_angle_received_time < TRUE_WIND_ANGLE_MAX_DATA_AGE_MS)
		{
			nmea_message_data_MWV.wind_angle = true_wind_angle_data;
			nmea_message_data_MWV.status = 'A';
			nmea_message_data_MWV.data_available |= (NMEA_MWV_WIND_ANGLE_PRESENT | NMEA_MWV_STATUS_PRESENT);
		}

		if (time_ms - boat_data_reception_time.true_wind_speed_received_time < TRUE_WIND_SPEED_MAX_DATA_AGE_MS)
		{
			nmea_message_data_MWV.wind_speed = true_wind_speed_data;
			nmea_message_data_MWV.status = 'A';
			nmea_message_data_MWV.data_available |= (NMEA_MWV_WIND_SPEED_PRESENT | NMEA_MWV_STATUS_PRESENT);
		}
	}
	else	
	{
		nmea_message_data_MWV.reference = 'R';

		if (time_ms - boat_data_reception_time.apparent_wind_angle_received_time < APPARENT_WIND_ANGLE_MAX_DATA_AGE_MS)
		{
			nmea_message_data_MWV.wind_angle = apparent_wind_angle_data;
			nmea_message_data_MWV.status = 'A';
			nmea_message_data_MWV.data_available |= (NMEA_MWV_WIND_ANGLE_PRESENT | NMEA_MWV_STATUS_PRESENT);
		}

		if (time_ms - boat_data_reception_time.apparent_wind_speed_received_time < APPARENT_WIND_SPEED_MAX_DATA_AGE_MS)
		{
			nmea_message_data_MWV.wind_speed = apparent_wind_speed_data;
			nmea_message_data_MWV.status = 'A';
			nmea_message_data_MWV.data_available |= (NMEA_MWV_WIND_SPEED_PRESENT | NMEA_MWV_STATUS_PRESENT);
		}
	}

	message_type_toggle = !message_type_toggle;
}

/* VLW transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_VLW = {nmea_message_VLW, PORT_BLUETOOTH, 1000UL, VLW_transmit_callback, &nmea_message_data_VLW, (nmea_encoder_function_t)nmea_encode_VLW};

static void VLW_transmit_callback(void)
{
	uint32_t time_ms = timer_get_time_ms();
	nmea_message_data_VLW.data_available = 0UL;

	if (time_ms - boat_data_reception_time.trip_received_time < TRIP_MAX_DATA_AGE_MS)
	{
		nmea_message_data_VLW.trip_water_distance = trip_data;
		nmea_message_data_VLW.data_available |= NMEA_VLW_TRIP_WATER_DISTANCE_PRESENT;
	}

	if (time_ms - boat_data_reception_time.total_distance_received_time < TOTAL_DISTANCE_MAX_DATA_AGE_MS)
	{
		nmea_message_data_VLW.total_water_distance = total_distance_data;
		nmea_message_data_VLW.data_available |= NMEA_VLW_TOTAL_WATER_DISTANCE_PRESENT;
	}
}

/* HDM transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_HDM = {nmea_message_HDM, PORT_BLUETOOTH, 1000UL, HDM_transmit_callback, &nmea_message_data_HDM, (nmea_encoder_function_t)nmea_encode_HDM};

static void HDM_transmit_callback(void)
{
	nmea_message_data_HDM.magnetic_heading = heading_true_data - variation_wmm_data;
	nmea_message_data_HDM.data_available = NMEA_HDM_MAG_HEADING_PRESENT;
}

/* HDT transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_HDT = {nmea_message_HDT, PORT_BLUETOOTH, 1000UL, HDT_transmit_callback, &nmea_message_data_HDT, (nmea_encoder_function_t)nmea_encode_HDT};

static void HDT_transmit_callback(void)
{
	nmea_message_data_HDT.true_heading = heading_true_data;
	nmea_message_data_HDT.data_available = NMEA_HDT_TRUE_HEADING_PRESENT;
}

/* VHW transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_VHW = {nmea_message_VHW,	PORT_BLUETOOTH, 1000UL, VHW_transmit_callback, &nmea_message_data_VHW, (nmea_encoder_function_t)nmea_encode_VHW};

static void VHW_transmit_callback(void)
{
	nmea_message_data_VHW.water_speed_knots = boat_speed_data;
	nmea_message_data_VHW.data_available = NMEA_VHW_WATER_SPEED_KTS_PRESENT;
}

/* MTW transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_MTW = {nmea_message_MTW, PORT_BLUETOOTH, 2000UL, MTW_transmit_callback, &nmea_message_data_MTW, (nmea_encoder_function_t)nmea_encode_MTW};

static void MTW_transmit_callback(void)
{
	nmea_message_data_MTW.water_temperature = seawater_temeperature_data;
	nmea_message_data_MTW.data_available = NMEA_MTW_WATER_TEMPERATURE_PRESENT;
}

/* DPT transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_DPT = {nmea_message_DPT,	PORT_BLUETOOTH, 500UL, DPT_transmit_callback, &nmea_message_data_DPT, (nmea_encoder_function_t)nmea_encode_DPT};

static void DPT_transmit_callback(void)
{
	nmea_message_data_DPT.depth = depth_data;
	nmea_message_data_DPT.data_available = NMEA_DPT_DEPTH_PRESENT;
}

/* GGA receive */
static const nmea_receive_message_details_t nmea_receive_message_details_GGA = {nmea_message_GGA, PORT_N0183, GGA_receive_callback};

static void GGA_receive_callback(char *data)
{
	if (nmea_decode_GGA(data, &nmea_message_data_GGA) == nmea_error_none)
	{
		nmea_transmit_message_now(PORT_BLUETOOTH, nmea_message_GGA);
	}
}

/* GGA - transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_GGA = {nmea_message_GGA,	PORT_BLUETOOTH, 0UL, GGA_transmit_callback, &nmea_message_data_GGA, (nmea_encoder_function_t)nmea_encode_GGA};

static void GGA_transmit_callback(void)
{
}

/* VDM - receive */
static const nmea_receive_message_details_t nmea_receive_message_details_VDM = {nmea_message_VDM, PORT_N0183, VDM_receive_callback};

static void VDM_receive_callback(char *data)
{
	if (nmea_decode_VDM(data, &nmea_message_data_VDM) == nmea_error_none)
	{
		nmea_transmit_message_now(PORT_BLUETOOTH, nmea_message_VDM);
	}
}

/* VDM - transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_VDM = {nmea_message_VDM,	PORT_BLUETOOTH, 0UL, VDM_transmit_callback, &nmea_message_data_VDM, (nmea_encoder_function_t)nmea_encode_VDM};

static void VDM_transmit_callback(void)
{
}

/* RMC receive */
static const nmea_receive_message_details_t nmea_receive_message_details_RMC = {nmea_message_RMC, PORT_N0183, RMC_receive_callback};

static void RMC_receive_callback(char *data)
{
	uint32_t time_ms = timer_get_time_ms();	
	
	if (nmea_decode_RMC(data, &nmea_message_data_RMC) == nmea_error_none)
	{		
		if (nmea_message_data_RMC.status == 'A')
		{
			if (nmea_message_data_RMC.data_available & NMEA_RMC_UTC_PRESENT)
			{
				gmt_data.hour = nmea_message_data_RMC.utc.hours;
				gmt_data.minute = nmea_message_data_RMC.utc.minutes;
				gmt_data.second = (uint8_t)nmea_message_data_RMC.utc.seconds;
				boat_data_reception_time.gmt_received_time = time_ms;
			}

			if (nmea_message_data_RMC.data_available & NMEA_RMC_DATE_PRESENT)
			{
				date_data.year = (uint8_t)(nmea_message_data_RMC.date.year - 2000U);
				date_data.month = nmea_message_data_RMC.date.month;
				date_data.date = nmea_message_data_RMC.date.date;
				boat_data_reception_time.date_received_time = time_ms;
			}

			if (nmea_message_data_RMC.data_available & NMEA_RMC_SOG_PRESENT)
			{
#ifndef FAKE_DATA				
				speed_over_ground_data = nmea_message_data_RMC.SOG;
				boat_data_reception_time.speed_over_ground_received_time = time_ms;				
#endif				
			}

			if (nmea_message_data_RMC.data_available & NMEA_RMC_COG_PRESENT)
			{
#ifndef FAKE_DATA								
				course_over_ground_data = nmea_message_data_RMC.COG;
				boat_data_reception_time.course_over_ground_received_time = time_ms;	
#endif				
			}
			else		// horrible hack because COG is not present when sog = 0
			{
#ifndef FAKE_DATA								
				course_over_ground_data = 0;
				boat_data_reception_time.course_over_ground_received_time = time_ms;
#endif				
			}

			if (nmea_message_data_RMC.data_available & NMEA_RMC_LATITUDE_PRESENT)
			{
#ifndef FAKE_DATA					
				frac_part = modff(nmea_message_data_RMC.latitude / 100.0f, &int_part);
				latitude_data = int_part;
				latitude_data += frac_part / 0.6f;
				boat_data_reception_time.latitude_received_time = time_ms;		
#endif
			}

			if (nmea_message_data_RMC.data_available & NMEA_RMC_LONGITUDE_PRESENT)
			{
#ifndef FAKE_DATA								
				frac_part = modff(nmea_message_data_RMC.longitude / 100.0f, &int_part);
				longitude_data = int_part;
				longitude_data += frac_part / 0.6f;
				boat_data_reception_time.longitude_received_time = time_ms;		
#endif			
			}
		}
	}
}

/* RMC transmit to VHF */
static const transmit_message_details_t nmea_transmit_message_details_RMC_bluetooth = {nmea_message_RMC, PORT_BLUETOOTH, 1000UL, RMC_transmit_callback, &nmea_message_data_RMC, (nmea_encoder_function_t)nmea_encode_RMC};

static void RMC_transmit_callback(void)
{
	float int_part;
	float frac_part;

	nmea_message_data_RMC.status = 'A';
	nmea_message_data_RMC.utc.seconds = (float)gmt_data.second;
	nmea_message_data_RMC.utc.minutes = gmt_data.minute;
	nmea_message_data_RMC.utc.hours = gmt_data.hour;
	nmea_message_data_RMC.date.year = (uint16_t)date_data.year + 2000U;
	nmea_message_data_RMC.date.month = date_data.month;
	nmea_message_data_RMC.date.date = date_data.date;
	nmea_message_data_RMC.SOG = speed_over_ground_data;
	nmea_message_data_RMC.COG = course_over_ground_data;
	frac_part = modff(latitude_data, &int_part);
	nmea_message_data_RMC.latitude = int_part * 100.0f;
	nmea_message_data_RMC.latitude += frac_part * 60.0f;
	frac_part = modff(longitude_data, &int_part);
	nmea_message_data_RMC.longitude = int_part * 100.0f;
	nmea_message_data_RMC.longitude += frac_part * 60.0f;
	nmea_message_data_RMC.mode = 'A';
	if (variation_wmm_data < 0.0f)
	{
		nmea_message_data_RMC.magnetic_variation = - variation_wmm_data;
		nmea_message_data_RMC.magnetic_variation_direction = 'W';
	}
	else
	{
		nmea_message_data_RMC.magnetic_variation = variation_wmm_data;
		nmea_message_data_RMC.magnetic_variation_direction = 'E';
	}
	nmea_message_data_RMC.navigation_status = 'S';
	nmea_message_data_RMC.data_available = NMEA_RMC_UTC_PRESENT | NMEA_RMC_STATUS_PRESENT |
			NMEA_RMC_SOG_PRESENT | NMEA_RMC_COG_PRESENT | NMEA_RMC_DATE_PRESENT | NMEA_RMC_LATITUDE_PRESENT |
			NMEA_RMC_LONGITUDE_PRESENT | NMEA_RMC_MODE_PRESENT | NMEA_RMC_NAV_STATUS_PRESENT | NMEA_RMC_MAG_VARIATION_PRESENT | NMEA_RMC_MAG_DIRECTION_PRESENT;
}

/* XDR transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_XDR = {nmea_message_XDR,	PORT_BLUETOOTH, 10000UL, XDR_transmit_callback,	&nmea_message_data_XDR,	(nmea_encoder_function_t)nmea_encode_XDR};

static void XDR_transmit_callback(void)
{
	nmea_message_data_XDR.measurements[0].decimal_places = 4U;
	nmea_message_data_XDR.measurements[0].transducer_type = 'P';
	nmea_message_data_XDR.measurements[0].transducer_id[0] = '\0';
	nmea_message_data_XDR.measurements[0].units = 'B';
	nmea_message_data_XDR.measurements[0].measurement = pressure_data / 1000.0f;
	nmea_message_data_XDR.data_available = NMEA_XDR_MEASUREMENT_1_PRESENT;
}

/* MDA transmit to OpenCPN */
static const transmit_message_details_t nmea_transmit_message_details_MDA = {nmea_message_MDA,	PORT_BLUETOOTH, 10000UL, MDA_transmit_callback,	&nmea_message_data_MDA,	(nmea_encoder_function_t)nmea_encode_MDA};

static void MDA_transmit_callback(void)
{
	nmea_message_data_MDA.pressure_bars = pressure_data / 1000.0f;
	nmea_message_data_MDA.data_available = NMEA_MDA_PRESSURE_BARS_PRESENT;
}

TaskHandle_t get_main_task_handle(void)
{
	return main_task_handle;
}

static void vTimerCallback25ms(TimerHandle_t xTimer)
{
	(void)xTimer;
	
	nmea_process();
}

static void vTimerCallback1s(TimerHandle_t xTimer)
{
	(void)xTimer;
	
	uint32_t time_ms = timer_get_time_ms();
	
	// update local time by 1 second (unless it's 23:59:59) if latest received time is more than 1 second old
	if (time_ms - boat_data_reception_time.gmt_received_time > 1000UL)
	{
		if (!(gmt_data.hour == 23U && gmt_data.minute == 59U && gmt_data.second == 59U))
		{
			gmt_data.second++;
			if (gmt_data.second > 59U)
			{
				gmt_data.second = 0U;
				gmt_data.minute++;
				if (gmt_data.minute > 59U)
				{
					gmt_data.hour++;
				}
			}
		}
	}	
	
    // switch on or off MWD message transmission here
	if ((time_ms - boat_data_reception_time.wind_direction_magnetic_received_time < WIND_DIRECTION_MAGNETIC_MAX_DATA_AGE_MS ||
			time_ms - boat_data_reception_time.wind_direction_magnetic_received_time < WIND_DIRECTION_TRUE_MAX_DATA_AGE_MS) &&
			time_ms - boat_data_reception_time.true_wind_speed_received_time < TRUE_WIND_SPEED_MAX_DATA_AGE_MS)
	{
		nmea_enable_transmit_message(&nmea_transmit_message_details_MWD);
	}
	else
	{
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_MWD);
	}	
	
    // switch on or off MWV message transmission here
	if (time_ms - boat_data_reception_time.apparent_wind_angle_received_time < APPARENT_WIND_ANGLE_MAX_DATA_AGE_MS ||
			time_ms - boat_data_reception_time.apparent_wind_speed_received_time < APPARENT_WIND_SPEED_MAX_DATA_AGE_MS ||
			time_ms - boat_data_reception_time.true_wind_angle_received_time < TRUE_WIND_ANGLE_MAX_DATA_AGE_MS ||
			time_ms - boat_data_reception_time.true_wind_speed_received_time < TRUE_WIND_SPEED_MAX_DATA_AGE_MS)
	{
		nmea_enable_transmit_message(&nmea_transmit_message_details_MWV);
	}
	else
	{
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_MWV);
	}

    // switch on or off VLW message transmission here
	if (time_ms - boat_data_reception_time.trip_received_time < TRIP_MAX_DATA_AGE_MS ||
			time_ms - boat_data_reception_time.total_distance_received_time < TOTAL_DISTANCE_MAX_DATA_AGE_MS)
	{
		nmea_enable_transmit_message(&nmea_transmit_message_details_VLW);
	}
	else
	{
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_VLW);
	}	
	
    // switch on or off HDM/HDT message transmission here
	if (time_ms - boat_data_reception_time.heading_true_received_time < HEADING_TRUE_MAX_DATA_AGE_MS)
	{
		nmea_enable_transmit_message(&nmea_transmit_message_details_HDT);

		if (time_ms - boat_data_reception_time.wmm_calculation_time < WMM_CALCULATION_MAX_DATA_AGE)
		{
			nmea_enable_transmit_message(&nmea_transmit_message_details_HDM);
		}
		else
		{
			nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_HDM);
		}
	}
	else
	{
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_HDM);
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_HDT);
	}	
	
    // switch on or off VHW message transmission here
	if (time_ms - boat_data_reception_time.boat_speed_received_time < BOAT_SPEED_MAX_DATA_AGE_MS)
	{
		nmea_enable_transmit_message(&nmea_transmit_message_details_VHW);
	}
	else
	{
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_VHW);
	}	
	
    // switch on or off MTW message transmission here
	if (time_ms - boat_data_reception_time.seawater_temperature_received_time < TEMPERATURE_MAX_DATA_AGE_MS)
	{
		nmea_enable_transmit_message(&nmea_transmit_message_details_MTW);
	}
	else
	{
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_MTW);
	}	
	
    // switch on or off DPT message transmission here
	if (time_ms - boat_data_reception_time.depth_received_time < DEPTH_MAX_DATA_AGE_MS)
	{
		nmea_enable_transmit_message(&nmea_transmit_message_details_DPT);
	}
	else
	{
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_DPT);
	}	
	
    // switch on or off RMC message transmission here
	if (time_ms - boat_data_reception_time.gmt_received_time < GMT_MAX_DATA_AGE_MS &&
			time_ms - boat_data_reception_time.date_received_time < DATE_MAX_DATA_AGE_MS &&
			time_ms - boat_data_reception_time.speed_over_ground_received_time < SOG_MAX_DATA_AGE_MS &&
			time_ms - boat_data_reception_time.course_over_ground_received_time < COG_MAX_DATA_AGE_MS &&
			time_ms - boat_data_reception_time.latitude_received_time < LATITUDE_MAX_DATA_AGE_MS &&
			time_ms - boat_data_reception_time.longitude_received_time < LONGITUDE_MAX_DATA_AGE_MS)
	{
		nmea_enable_transmit_message(&nmea_transmit_message_details_RMC_bluetooth);
	}
	else
	{
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_RMC);
	}	
	
	// switch on or off XDR and MDA message transmission here
	if (time_ms - boat_data_reception_time.pressure_received_time < PRESSURE_MAX_DATA_AGE_MS)
	{
		nmea_enable_transmit_message(&nmea_transmit_message_details_XDR);
		nmea_enable_transmit_message(&nmea_transmit_message_details_MDA);
	}
	else
	{
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_XDR);
		nmea_disable_transmit_message(PORT_BLUETOOTH, nmea_message_MDA);
	}			
	
	if (settings_get_boat_iot_started())
	{
		led_flash(50UL);
	}	
}

static void vTimerCallback8s(TimerHandle_t xTimer)
{
	float wmm_date;
	float variation_wmm_data_temp;
	uint32_t time_ms = timer_get_time_ms();

	(void)xTimer;
	
	// check if a pressure reading is available
	if (xQueueReceive(pressure_sensor_queue_handle, (void *)&pressure_data, (TickType_t)0) == pdTRUE)
	{          
		tN2kMsg N2kMsg;
		SetN2kOutsideEnvironmentalParameters(N2kMsg, 1U, N2kDoubleNA, N2kDoubleNA, mBarToPascal((double)pressure_data));
		NMEA2000.SendMsg(N2kMsg);
		boat_data_reception_time.pressure_received_time = timer_get_time_ms();
	}	

    // check if it's time to do a wmm calculation and if it is check that required parameters are fresh
	if (time_ms - boat_data_reception_time.wmm_calculation_time > WMM_CALCULATION_MAX_DATA_AGE &&
			time_ms - boat_data_reception_time.latitude_received_time < LATITUDE_MAX_DATA_AGE_MS &&
			time_ms - boat_data_reception_time.longitude_received_time < LONGITUDE_MAX_DATA_AGE_MS &&
			time_ms - boat_data_reception_time.date_received_time < DATE_MAX_DATA_AGE_MS)
	{
		// it's time and every parameter is fresh so do a wmm calculation

		// convert date into format required by wmm
		wmm_date = wmm_get_date(date_data.year, date_data.month, date_data.date);

		// do the calculation
		E0000(latitude_data, longitude_data, wmm_date, &variation_wmm_data_temp);
		variation_wmm_data = variation_wmm_data_temp;
		
		// save this calculation time
		boat_data_reception_time.wmm_calculation_time = time_ms;
	}
}

static void HandleNMEA2000Msg(const tN2kMsg &N2kMsg) 
{	
	for (uint32_t i = 0UL; i < (uint32_t)(sizeof(NMEA2000Handlers) / sizeof(tNMEA2000Handler)); i++)
	{
		if (N2kMsg.PGN == NMEA2000Handlers[i].PGN)
		{
			if (NMEA2000Handlers[i].Handler != NULL)
			{
				NMEA2000Handlers[i].Handler(N2kMsg);
			}
			break;
		}
	}
}

static void heading_handler(const tN2kMsg &N2kMsg) 
{
    unsigned char SID;
    tN2kHeadingReference HeadingReference;
    double Heading;
    double Deviation;
    double Variation;
	
    if (ParseN2kHeading(N2kMsg, SID, Heading, Deviation, Variation, HeadingReference)) 
	{
		if (HeadingReference == N2khr_true)
		{
			if (!N2kIsNA(Heading))
			{
				heading_true_data = (float)RadToDeg(Heading);
				boat_data_reception_time.heading_true_received_time = timer_get_time_ms();
			}
		}
		else if (HeadingReference == N2khr_magnetic)
		{
			if (!N2kIsNA(Heading))
			{
				if (timer_get_time_ms() - boat_data_reception_time.wmm_calculation_time < WMM_CALCULATION_MAX_DATA_AGE)
				{
					heading_true_data = (float)RadToDeg(Heading) + variation_wmm_data;
					boat_data_reception_time.heading_true_received_time = timer_get_time_ms();				
				}
			}				
		}
	}
}	

static void depth_handler(const tN2kMsg &N2kMsg) 
{
    unsigned char SID;
    double depth_below_transducer;
    double offset;

	if (ParseN2kWaterDepth(N2kMsg, SID, depth_below_transducer, offset)) 
	{
		if (!N2kIsNA(depth_below_transducer) && !N2kIsNA(offset))
		{
			depth_data = (float)(depth_below_transducer + offset);
			boat_data_reception_time.depth_received_time = timer_get_time_ms();
		}
	}
}

static void boat_speed_handler(const tN2kMsg &N2kMsg) 
{
    unsigned char SID;
    double SOW;
    double SOG;
    tN2kSpeedWaterReferenceType SWRT;
	
    if (ParseN2kBoatSpeed(N2kMsg, SID, SOW, SOG, SWRT)) 
	{
		if (!N2kIsNA(SOW) && SWRT != N2kSWRT_Error && SWRT != N2kSWRT_Unavailable)
		{
			boat_speed_data = (float)msToKnots(SOW);
			boat_data_reception_time.boat_speed_received_time = timer_get_time_ms();
		}
    }	
}	

static void wind_handler(const tN2kMsg &N2kMsg) 
{
    unsigned char SID;
	double WindSpeed;
	double WindAngle;
	tN2kWindReference WindReference;
	
    if (ParseN2kWindSpeed(N2kMsg, SID, WindSpeed, WindAngle, WindReference)) 
	{
		if (WindReference == N2kWind_Apparent)
		{
			if (!N2kIsNA(WindSpeed))
			{
				apparent_wind_speed_data = (float)msToKnots(WindSpeed);
				boat_data_reception_time.apparent_wind_speed_received_time = timer_get_time_ms();
			}
			
			if (!N2kIsNA(WindAngle))
			{
				apparent_wind_angle_data = (float)RadToDeg(WindAngle);
				boat_data_reception_time.apparent_wind_angle_received_time = timer_get_time_ms();
			}			
		}
		else if (WindReference == N2kWind_True_water)
		{
			if (!N2kIsNA(WindSpeed))
			{
				true_wind_speed_data = (float)msToKnots(WindSpeed);
				boat_data_reception_time.true_wind_speed_received_time = timer_get_time_ms();
			}
			
			if (!N2kIsNA(WindAngle))
			{
				true_wind_angle_data = (float)RadToDeg(WindAngle);
				boat_data_reception_time.true_wind_angle_received_time = timer_get_time_ms();
			}			
		}	
		else if (WindReference == N2kWind_True_North)
		{
			if (!N2kIsNA(WindSpeed))
			{
				true_wind_speed_data = (float)msToKnots(WindSpeed);
				boat_data_reception_time.true_wind_speed_received_time = timer_get_time_ms();
			}
			
			if (!N2kIsNA(WindAngle))
			{
				wind_direction_true_data = (float)RadToDeg(WindAngle);
				boat_data_reception_time.wind_direction_true_received_time = timer_get_time_ms();
			}				
		}
		else if (WindReference == N2kWind_Magnetic)
		{
			if (!N2kIsNA(WindSpeed))
			{
				true_wind_speed_data = (float)msToKnots(WindSpeed);
				boat_data_reception_time.true_wind_speed_received_time = timer_get_time_ms();
			}
			
			if (!N2kIsNA(WindAngle))
			{
				wind_direction_magnetic_data = (float)RadToDeg(WindAngle);
				boat_data_reception_time.wind_direction_magnetic_received_time = timer_get_time_ms();
			}				
		}			
    }	
}	

static void log_handler(const tN2kMsg &N2kMsg) 
{
	uint16_t DaysSince1970;
	double SecondsSinceMidnight;
	uint32_t Log;
	uint32_t TripLog;
	
    if (ParseN2kDistanceLog(N2kMsg, DaysSince1970, SecondsSinceMidnight, Log, TripLog)) 
	{
		if (!N2kIsNA(Log))
		{
			trip_data = ((float)TripLog) / 1852.0f;
			boat_data_reception_time.trip_received_time = timer_get_time_ms();
		}
		
		if (!N2kIsNA(Log))
		{
			total_distance_data = ((float)Log) / 1852.0f;
			boat_data_reception_time.total_distance_received_time = timer_get_time_ms();
		}			
    }	
}	

static void environmental_handler(const tN2kMsg &N2kMsg) 
{
    unsigned char SID;
	double WaterTemperature;
	double OutsideAmbientAirTemperature;
	double AtmosphericPressure;

	if (ParseN2kOutsideEnvironmentalParameters(N2kMsg, SID, WaterTemperature, OutsideAmbientAirTemperature, AtmosphericPressure)) 
	{
		if (!N2kIsNA(WaterTemperature))
		{
			seawater_temeperature_data = (float)KelvinToC(WaterTemperature);
			boat_data_reception_time.seawater_temperature_received_time = timer_get_time_ms();
		}
	}
}

extern "C" void app_main(void)
{
	uint8_t task_started_count = 0U;
    
    main_task_handle = xTaskGetCurrentTaskHandle();
    pressure_sensor_init();
	serial_init(38400UL);
	led_init();
	wmm_init();
	settings_init();
	sms_init();
	
#ifdef FAKE_DATA
	depth_data = 3.0f;	
	heading_true_data = 80.0f;
	course_over_ground_data = 220.0f;
	trip_data = 0.1f;
	total_distance_data = 32445.0;
	boat_speed_data = 0.0f;
	speed_over_ground_data = 0.0f;
	seawater_temeperature_data = 6.5f;
	latitude_data = 58.251f;
	longitude_data = -5.227f;	
	true_wind_speed_data = 18.0f;
	true_wind_angle_data = 80.0f;
	apparent_wind_speed_data = 18.0f;
	apparent_wind_angle_data = 80.0f;
#endif	
	
	// init all the reception times to some time a long time ago
	(void)memset((void *)&boat_data_reception_time, 0x7f, sizeof(boat_data_reception_time));
	
    // pressure sensor task
	pressure_sensor_queue_handle = xQueueCreateStatic((UBaseType_t)1, (UBaseType_t)(sizeof(float)), pressure_sensor_queue_buffer, &pressure_sensor_queue);
    (void)xTaskCreate(pressure_sensor_task, "pressure sensor task", PRESSURE_SENSOR_TASK_STACK_SIZE, &pressure_sensor_queue_handle, (UBaseType_t)1, NULL); 
	
    // boat iot task
    (void)xTaskCreate(boat_iot_task, "boat iot task", BOAT_IOT_TASK_STACK_SIZE, NULL, (UBaseType_t)1, NULL); 	

	// wait until all server tasks have started
	do
	{
		(void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		task_started_count++;
	}
	while (task_started_count < 2U);  	
	
    ESP_LOGI(pcTaskGetName(NULL), "All tasks started");
	
    ESP_LOGI(pcTaskGetName(NULL), "Device NMEA2000 address: %u", (uint32_t)settings_get_device_address());
    ESP_LOGI(pcTaskGetName(NULL), "APN: %s", settings_get_apn());
    ESP_LOGI(pcTaskGetName(NULL), "User name: %s", settings_get_apn_user_name());
    ESP_LOGI(pcTaskGetName(NULL), "Password: %s", settings_get_apn_password());
    ESP_LOGI(pcTaskGetName(NULL), "Broker address: %s", settings_get_mqtt_broker_address());
    ESP_LOGI(pcTaskGetName(NULL), "Broker port: %u", (uint32_t)settings_get_mqtt_broker_port());

    // set up N2K  
    NMEA2000.SetN2kCANMsgBufSize(16);
    NMEA2000.SetProductInformation("00000001", 1, "BlueBridge", "1.0", "BB1.0");		
    NMEA2000.SetDeviceInformation(1, 140, 75, 2040); 
    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, settings_get_device_address());
    NMEA2000.EnableForward(false);      
	NMEA2000.SetN2kCANMsgBufSize(25);
    NMEA2000.ExtendTransmitMessages(n2k_transmit_messages);
	NMEA2000.ExtendReceiveMessages(n2k_receive_messages);
	NMEA2000.SetMsgHandler(HandleNMEA2000Msg);	
    NMEA2000.Open();	
	
    nmea_enable_receive_message(&nmea_receive_message_details_RMC);	
	nmea_enable_receive_message(&nmea_receive_message_details_VDM);
	nmea_enable_receive_message(&nmea_receive_message_details_GGA);
	nmea_enable_transmit_message(&nmea_transmit_message_details_VDM);
	nmea_enable_transmit_message(&nmea_transmit_message_details_GGA);
	
	// create 25ms timer
	xTimers[SW_TIMER_25_MS] = xTimerCreate(
			"25ms timer",
			(TickType_t)25,		
			pdTRUE,
			(void *)0,
			vTimerCallback25ms);		
	
	// create 1s timer
	xTimers[SW_TIMER_1_S] = xTimerCreate(
			"1s timer",
			(TickType_t)1000,
			pdTRUE,
			(void *)0,
			vTimerCallback1s);	
			
	// create 8s timer
	xTimers[SW_TIMER_8_S] = xTimerCreate(
			"8s timer",
			(TickType_t)8000,
			pdTRUE,
			(void *)0,
			vTimerCallback8s);				
			
	(void)xTimerStart(xTimers[SW_TIMER_25_MS], (TickType_t)0);						
	(void)xTimerStart(xTimers[SW_TIMER_1_S], (TickType_t)0);			
	(void)xTimerStart(xTimers[SW_TIMER_8_S], (TickType_t)0);		
	
	while (true) 
	{ 
        // do N2K routine stuff
		NMEA2000.ParseMessages();
        if (NMEA2000.ReadResetAddressChanged())
        {
			settings_set_device_address(NMEA2000.GetN2kSource());
			settings_save();
        }	
		
		vTaskDelay(10);        
				
#ifdef FAKE_DATA
		fake_data();
#endif						
    }		
}

#ifdef FAKE_DATA
static void fake_data(void)
{
	static uint32_t i;

	i++;
	
	if (i % 100UL == 0UL)
	{
		depth_data += (0.2f * (float)esp_random() / (float)UINT32_MAX) - 0.1f;
		if (depth_data < 2.0f)
		{
			depth_data = 2.0f;
		}
		if (depth_data > 4.0f)
		{
			depth_data = 4.0f;
		}		
		boat_data_reception_time.depth_received_time = timer_get_time_ms();		
		
		heading_true_data += (10.0f * (float)esp_random() / (float)UINT32_MAX) - 5.0f;
		if (heading_true_data < 60.0f)
		{
			heading_true_data = 60.0f;
		}
		if (heading_true_data > 100.0f)
		{
			heading_true_data = 100.0f;
		}		
		boat_data_reception_time.heading_true_received_time = timer_get_time_ms();		
		
		course_over_ground_data += (90.0f * (float)esp_random() / (float)UINT32_MAX) - 45.0f;
		if (course_over_ground_data < 0.0f)
		{
			course_over_ground_data += 360.0f;
		}
		if (course_over_ground_data >= 360.0f)
		{
			course_over_ground_data -= 360.0f;
		}		
		boat_data_reception_time.course_over_ground_received_time = timer_get_time_ms();				
		
		boat_speed_data += (0.1f * (float)esp_random() / (float)UINT32_MAX) - 0.05f;
		if (boat_speed_data < 0.0f)
		{
			boat_speed_data = 0.0f;
		}
		if (boat_speed_data > 0.2f)
		{
			boat_speed_data = 0.2f;
		}		
		boat_data_reception_time.boat_speed_received_time = timer_get_time_ms();		

		speed_over_ground_data += (0.1f * (float)esp_random() / (float)UINT32_MAX) - 0.05f;
		if (speed_over_ground_data < 0.0f)
		{
			speed_over_ground_data = 0.0f;
		}
		if (speed_over_ground_data > 0.2f)
		{
			speed_over_ground_data = 0.2f;
		}		
		boat_data_reception_time.speed_over_ground_received_time = timer_get_time_ms();	
		
		seawater_temeperature_data += (0.1f * (float)esp_random() / (float)UINT32_MAX) - 0.05f;
		if (seawater_temeperature_data < 6.0f)
		{
			seawater_temeperature_data = 6.0f;
		}
		if (seawater_temeperature_data > 7.0f)
		{
			seawater_temeperature_data = 7.0f;
		}		
		boat_data_reception_time.seawater_temperature_received_time = timer_get_time_ms();			

		true_wind_speed_data += (10.1f * (float)esp_random() / (float)UINT32_MAX) - 5.0f;
		if (true_wind_speed_data < 2.3f)
		{
			true_wind_speed_data = 2.3f;
		}
		if (true_wind_speed_data > 25.1f)
		{
			true_wind_speed_data = 25.1f;
		}		
		boat_data_reception_time.true_wind_speed_received_time = timer_get_time_ms();			
		
		apparent_wind_speed_data = true_wind_speed_data + (1.0f * (float)esp_random() / (float)UINT32_MAX) - 0.5f;		
		boat_data_reception_time.apparent_wind_speed_received_time = timer_get_time_ms();			
				
		true_wind_angle_data = heading_true_data + (5.1f * (float)esp_random() / (float)UINT32_MAX) - 2.5f;	
		boat_data_reception_time.apparent_wind_angle_received_time = timer_get_time_ms();			
		
		apparent_wind_angle_data = true_wind_angle_data + (8.0f * (float)esp_random() / (float)UINT32_MAX) - 4.0f;		
		boat_data_reception_time.apparent_wind_angle_received_time = timer_get_time_ms();				

		boat_data_reception_time.trip_received_time = timer_get_time_ms();
		boat_data_reception_time.total_distance_received_time = timer_get_time_ms();	
		boat_data_reception_time.latitude_received_time = timer_get_time_ms();
		boat_data_reception_time.longitude_received_time = timer_get_time_ms();		
	}
}
#endif