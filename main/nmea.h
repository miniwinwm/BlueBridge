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

#ifndef NMEA_H
#define NMEA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define NMEA_NUMBER_OF_PORTS                    			2U
#define NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS           	16U
#define NMEA_MAXIMUM_RECEIVE_MESSAGE_DETAILS           		6U
#define NMEA_MAX_MESSAGE_LENGTH     						82U
#define NMEA_MIN_MESSAGE_LENGTH     						9U
#define NMEA_APB_STATUS1_PRESENT                     		0x00000001UL
#define NMEA_APB_STATUS2_PRESENT                         	0x00000002UL
#define NMEA_APB_CROSS_TRACK_ERROR_PRESENT               	0x00000004UL
#define NMEA_APB_DIRECTION_TO_STEER_PRESENT              	0x00000008UL
#define NMEA_APB_ARRIVAL_CIRCLE_ENTERED_PRESENT          	0x00000010UL
#define NMEA_APB_PERPENDICULAR_PASSED_PRESENT            	0x00000020UL
#define NMEA_APB_BEARING_ORIG_TO_DEST_PRESENT            	0x00000040UL
#define NMEA_APB_BEARING_MAG_OR_TRUE_PRESENT             	0x00000080UL
#define NMEA_APB_DEST_WAYPOINT_ID_PRESENT                	0x00000100UL
#define NMEA_APB_BEARING_POS_TO_DEST_PRESENT             	0x00000200UL
#define NMEA_APB_BEARING_POS_TO_DEST_MAG_OR_TRUE_PRESENT 	0x00000400UL
#define NMEA_APB_HEADING_TO_STEER_PRESENT                	0x00000800UL
#define NMEA_APB_HEADING_TO_STEER_MAG_OR_TRUE_PRESENT    	0x00001000UL
#define NMEA_APB_MODE_PRESENT                            	0x00002000UL
#define NMEA_APB_WAYPOINT_NAME_MAX_LENGTH                	20U
#define NMEA_DPT_DEPTH_PRESENT                				0x00000001UL
#define NMEA_DPT_DEPTH_OFFSET_PRESENT         				0x00000002UL
#define NMEA_DPT_DEPTH_MAX_RANGE_PRESENT      				0x00000004UL
#define NMEA_GGA_UTC_PRESENT                  				0x00000001UL
#define NMEA_GGA_LATITUDE_PRESENT             				0x00000002UL
#define NMEA_GGA_LONGITUDE_PRESENT            				0x00000004UL
#define NMEA_GGA_QUALITY_INDICATOR_PRESENT    				0x00000008UL
#define NMEA_GGA_SATELLITES_IN_USE_PRESENT    				0x00000010UL
#define NMEA_GGA_HDOP_PRESENT                 				0x00000020UL
#define NMEA_GGA_ALTITUDE_PRESENT             				0x00000040UL
#define NMEA_GGA_GEIODAL_SEPARATION_PRESENT   				0x00000080UL
#define NMEA_GGA_DGPS_AGE_PRESENT             				0x00000100UL
#define NMEA_GGA_DGPS_STATION_ID_PRESENT      				0x00000200UL
#define NMEA_HDM_MAG_HEADING_PRESENT  						0x00000001UL
#define NMEA_HDT_TRUE_HEADING_PRESENT  						0x00000001UL
#define NMEA_MTW_WATER_TEMPERATURE_PRESENT 					0x00000001UL
#define NMEA_MWD_WIND_DIRECTION_TRUE_PRESENT 				0x00000001UL
#define NMEA_MWD_WIND_DIRECTION_MAG_PRESENT   				0x00000002UL
#define NMEA_MWD_WIND_SPEED_KTS_PRESENT       				0x00000004UL
#define NMEA_MWD_WIND_SPEED_MPS_PRESENT       				0x00000008UL
#define NMEA_MWV_WIND_ANGLE_PRESENT       					0x00000001UL
#define NMEA_MWV_REFERENCE_PRESENT        					0x00000002UL
#define NMEA_MWV_WIND_SPEED_PRESENT       					0x00000004UL
#define NMEA_MWV_WIND_SPEED_UNITS_PRESENT 					0x00000008UL
#define NMEA_MWV_STATUS_PRESENT           					0x00000010UL
#define NMEA_RMB_STATUS_PRESENT              				0x00000001UL
#define NMEA_RMB_CROSS_TRACK_ERROR_PRESENT   				0x00000002UL
#define NMEA_RMB_DIR_TO_STEER_PRESENT         				0x00000004UL
#define NMEA_RMB_ORIG_WAYPOINT_ID_PRESENT     				0x00000008UL
#define NMEA_RMB_DEST_WAYPOINT_ID_PRESENT     				0x00000010UL
#define NMEA_RMB_LATITUDE_PRESENT             				0x00000020UL
#define NMEA_RMB_LONGITUDE_PRESENT            				0x00000040UL
#define NMEA_RMB_RANGE_TO_DEST_PRESENT        				0x00000080UL
#define NMEA_RMB_BEARING_TRUE_PRESENT         				0x00000100UL
#define NMEA_RMB_VELOCITY_PRESENT             				0x00000200UL
#define NMEA_RMB_ARRIVAL_STATUS_PRESENT       				0x00000400UL
#define NMEA_RMB_MODE_PRESENT                 				0x00000800UL
#define NMEA_RMB_WAYPOINT_NAME_MAX_LENGTH     				9U
#define NMEA_RMC_UTC_PRESENT            					0x00000001UL
#define NMEA_RMC_STATUS_PRESENT         					0x00000002UL
#define NMEA_RMC_LATITUDE_PRESENT      						0x00000004UL
#define NMEA_RMC_LONGITUDE_PRESENT      					0x00000008UL
#define NMEA_RMC_SOG_PRESENT            					0x00000010UL
#define NMEA_RMC_COG_PRESENT            					0x00000020UL
#define NMEA_RMC_DATE_PRESENT          	 					0x00000040UL
#define NMEA_RMC_MAG_VARIATION_PRESENT  					0x00000080UL
#define NMEA_RMC_MAG_DIRECTION_PRESENT  					0x00000100UL
#define NMEA_RMC_MODE_PRESENT           					0x00000200UL
#define NMEA_RMC_NAV_STATUS_PRESENT     					0x00000400UL
#define NMEA_VDM_MAX_AIS_DATA_FIELD_LENGTH					62U
#define NMEA_VDM_FRAGMENT_COUNT_PRESENT  					0x00000001UL
#define NMEA_VDM_FRAGMENT_NUMBER_PRESENT  					0x00000002UL
#define NMEA_VDM_MESSAGE_IDENTIFIER_PRESENT  				0x00000004UL
#define NMEA_VDM_CHANNEL_CODE_PRESENT  						0x00000008UL
#define NMEA_VDM_DATA_PRESENT  								0x00000010UL
#define NMEA_VDM_FILL_BITS_PRESENT  						0x00000020UL
#define NMEA_VHW_HEADING_TRUE_PRESENT     					0x00000001UL
#define NMEA_VHW_HEADING_MAG_PRESENT      					0x00000002UL
#define NMEA_VHW_WATER_SPEED_KTS_PRESENT  					0x00000004UL
#define NMEA_VHW_WATER_SPEED_KMPH_PRESENT 					0x00000008UL
#define NMEA_VLW_TOTAL_WATER_DISTANCE_PRESENT        		0x00000001UL
#define NMEA_VLW_TRIP_WATER_DISTANCE_PRESENT         		0x00000002UL
#define NMEA_VLW_TOTAL_GROUND_DISTANCE_PRESENT       		0x00000004UL
#define NMEA_VLW_TRIP_GROUND_DISTANCE_PRESENT        		0x00000008UL
#define NMEA_MDA_PRESSURE_INCHES_PRESENT        			0x00000001UL
#define NMEA_MDA_PRESSURE_BARS_PRESENT        				0x00000002UL
#define NMEA_MDA_AIR_TEMPERATURE_PRESENT        			0x00000004UL
#define NMEA_MDA_WATER_TEMPERATURE_PRESENT        			0x00000008UL
#define NMEA_MDA_RELATIVE_HUMIDITY_PRESENT        			0x000000010UL
#define NMEA_MDA_ABSOLUTE_HUMIDITY_PRESENT        			0x000000020UL
#define NMEA_MDA_DEW_POINT_PRESENT        					0x000000040UL
#define NMEA_MDA_WIND_DIRECTION_TRUE_PRESENT        		0x000000080UL
#define NMEA_MDA_WIND_DIRECTION_MAGNETIC_PRESENT        	0x000000100UL
#define NMEA_MDA_WINDSPEED_KNOTS_PRESENT        			0x000000200UL
#define NMEA_MDA_WINDSPEED_MPS_PRESENT        				0x000000400UL
#define NMEA_XDR_MAX_MEASUREMENTS_COUNT            			6U
#define NMEA_XDR_MEASUREMENT_1_PRESENT                		0x00000001UL
#define NMEA_XDR_MEASUREMENT_2_PRESENT                		0x00000002UL
#define NMEA_XDR_MEASUREMENT_3_PRESENT                		0x00000004UL
#define NMEA_XDR_MEASUREMENT_4_PRESENT                		0x00000008UL
#define NMEA_XDR_MEASUREMENT_5_PRESENT                		0x00000010UL
#define NMEA_XDR_MEASUREMENT_6_PRESENT                		0x00000020UL
#define NMEA_XDR_MAX_ID_LENGTH                      		8U
#define NMEA_SPEED_UP_MESSAGE_PERMIL_PERIOD_ADJUSTMENT		999UL
#define NMEA_SLOW_DOWN_MESSAGE_PERMIL_PERIOD_ADJUSTMENT		1010UL

typedef struct
{
    uint8_t hours;          /* 0-23 */
    uint8_t minutes;        /* 0-59 */
    float seconds;        	/* 0-59.9999 */
} nmea_utc_time_t;

typedef struct
{
    uint8_t date;           /* 1 to 31 */
    uint8_t month;          /* 1 to 12 */
    uint16_t year;          /* 4 digit form */
} nmea_date_t;

typedef enum
{
    nmea_error_none,
	nmea_error_param,
    nmea_error_port,
    nmea_error_message,
	nmea_error_overflow,
    nmea_error_no_space,
} nmea_error_t;

typedef enum
{
    nmea_message_min,            /* must be first value */
    nmea_message_APB,
    nmea_message_DPT,
    nmea_message_HDT,
    nmea_message_HDM,
    nmea_message_GGA,
    nmea_message_MWD,
    nmea_message_MWV,
    nmea_message_MTW,
    nmea_message_RMB,
    nmea_message_RMC,
    nmea_message_VDM,
    nmea_message_VHW,
    nmea_message_VLW,
    nmea_message_XDR,
	nmea_message_MDA,
    nmea_message_max             /* must be last value */
} nmea_message_type_t;

typedef void (*nmea_receive_message_callback_t)(char *message);
typedef void (*nmea_get_transmit_data_callback_t)(void);
typedef nmea_error_t (*nmea_encoder_function_t)(const char *message_data, void *source);

typedef struct
{
    nmea_message_type_t message_type;
    uint8_t port;
    uint32_t transmit_period_ms;
    nmea_get_transmit_data_callback_t get_transmit_data_callback;
    void *get_data_structure;
    nmea_encoder_function_t encoder;
} transmit_message_details_t;

typedef struct
{
    nmea_message_type_t message_type;
    uint8_t port;
    nmea_receive_message_callback_t receive_message_callback;
} nmea_receive_message_details_t;

typedef struct
{
    char status1;
    char status2;
    uint32_t data_available;
    float cross_track_error;
    char direction_to_steer;
    char arrival_circle_entered;
    char perpendicular_passed;
    char bearing_magnetic_or_true;
    float bearing_origin_to_destination;
    float bearing_position_to_destination;
    float heading_to_steer;
    char waypoint_name[NMEA_APB_WAYPOINT_NAME_MAX_LENGTH + 1];
    char bearing_position_to_destination_magnetic_or_true;
    char heading_to_steer_magnetic_or_true;
    char mode;
} nmea_message_data_APB_t;

typedef struct
{
    uint32_t data_available;
    float depth;
    float depth_offset;
    float depth_maximum_range;
} nmea_message_data_DPT_t;

typedef struct
{
    uint8_t quality_indicator;
    uint8_t satellites_in_use;
    uint32_t data_available;
    nmea_utc_time_t utc;
    uint16_t dgps_station_id;
    float latitude;
    float longitude;
    float HDOP;
    float altitude;
    float geoidal_separation;
    float dgpsAge;
} nmea_message_data_GGA_t;

typedef struct
{
    uint32_t data_available;
    float magnetic_heading;
} nmea_message_data_HDM_t;
typedef struct
{
    uint32_t data_available;
    float true_heading;
} nmea_message_data_HDT_t;

typedef struct
{
    uint32_t data_available;
    float water_temperature;
} nmea_message_data_MTW_t;

typedef struct
{
    uint32_t data_available;
    float wind_direction_true;
    float wind_direction_magnetic;
    float wind_speed_knots;
    float wind_speed_mps;
} nmea_message_data_MWD_t;

typedef struct
{
    char wind_speed_units;
    char status;
    uint32_t data_available;
    float wind_angle;
    float wind_speed;
    char reference;
} nmea_message_data_MWV_t;

typedef struct
{
    char status;
    char direction_to_steer;
    uint32_t data_available;
    float cross_track_error;
    char origin_waypoint_name[NMEA_RMB_WAYPOINT_NAME_MAX_LENGTH + 1];
    char destination_waypoint_name[NMEA_RMB_WAYPOINT_NAME_MAX_LENGTH + 1];
    char arrivalStatus;
    char mode;
    float latitude;
    float longitude;
    float range_to_destination;
    float bearing_true;
    float velocity;
} nmea_message_data_RMB_t;

typedef struct
{
    char magnetic_variation_direction;
    char status;
    uint32_t data_available;
    nmea_utc_time_t utc;
    char mode;
    char navigation_status;
    float latitude;
    float longitude;
    float SOG;
    float COG;
    nmea_date_t date;
    float magnetic_variation;
} nmea_message_data_RMC_t;

typedef struct
{
    char channel_code;
    uint8_t fill_bits;
    uint32_t data_available;
    uint8_t fragment_count;
    uint8_t fragment_number;
    uint8_t message_identifier;
    char data[NMEA_VDM_MAX_AIS_DATA_FIELD_LENGTH + 1];
} nmea_message_data_VDM_t;

typedef struct
{
    uint32_t data_available;
    float heading_true;
    float heading_magnetic;
    float water_speed_knots;
    float water_speed_kmph;
} nmea_message_data_VHW_t;

typedef struct
{
    uint32_t data_available;
    float pressure_inches;
    float pressure_bars;
    float air_temperature;
    float water_temperature;
    float relative_huimidity;
    float absolute_humidity;
    float dew_point;
    float wind_direction_true;
    float wind_direction_magnetic;
    float windspeed_knots;
    float windspeed_mps;
} nmea_message_data_MDA_t;

typedef struct
{
    uint32_t data_available;
    float total_water_distance;
    float trip_water_distance;
    float total_ground_distance;
    float trip_ground_distance;
} nmea_message_data_VLW_t;

typedef struct
{
    char transducer_type;
    char transducer_id[NMEA_XDR_MAX_ID_LENGTH+1];
    char units;
    uint8_t decimal_places;
    float measurement;
} nmea_XDR_tuple_t;

typedef struct
{
    uint32_t data_available;
    nmea_XDR_tuple_t measurements[NMEA_XDR_MAX_MEASUREMENTS_COUNT];
} nmea_message_data_XDR_t;

void nmea_enable_transmit_message(const transmit_message_details_t *nmea_transmit_message_details);
void nmea_disable_transmit_message(uint8_t port, nmea_message_type_t message_type);
void nmea_enable_receive_message(const nmea_receive_message_details_t *nmea_receive_message_details);
void nmea_transmit_message_now(uint8_t port, nmea_message_type_t message_type);
void nmea_process(void);
uint8_t nmea_count_set_bits(uint32_t n, uint8_t start_bit, uint8_t length);
nmea_error_t nmea_decode_APB(char *message_data, nmea_message_data_APB_t *result);
nmea_error_t nmea_decode_GGA(char *message_data, nmea_message_data_GGA_t *result);
nmea_error_t nmea_encode_GGA(char *message_data,  const void *source);
nmea_error_t nmea_encode_HDM(char *message_data, const void *source);
nmea_error_t nmea_encode_MTW(char *message_data, const void *source);
nmea_error_t nmea_encode_DPT(char *message_data, const void *source);
nmea_error_t nmea_encode_MWD(char *message_data, const void *source);
nmea_error_t nmea_encode_MWV(char *message_data, const void *source);
nmea_error_t nmea_decode_RMC(char *message_data, nmea_message_data_RMC_t *result);
nmea_error_t nmea_encode_RMC(char *message_data, const void *source);
nmea_error_t nmea_decode_VDM(char *message_data, nmea_message_data_VDM_t *result);
nmea_error_t nmea_encode_VDM(char *message_data, const void *source);
nmea_error_t nmea_decode_RMB(char *message_data, nmea_message_data_RMB_t *result);
nmea_error_t nmea_encode_VLW(char *message_data, const void *source);
nmea_error_t nmea_encode_VHW(char *message_data,  const void *source);
nmea_error_t nmea_encode_XDR(char *message_data, const void *source);
nmea_error_t nmea_encode_HDT(char *message_data, const void *source);
nmea_error_t nmea_encode_MDA(char *message_data, const void *source);

#ifdef __cplusplus
}
#endif

#endif
