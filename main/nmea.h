/*

MIT License

Copyright (c) John Blaiklock 2022 BlueBridge

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

/***************
*** INCLUDES ***
***************/

#include <stdint.h>

/**************
*** DEFINES ***
**************/

#define NMEA_NUMBER_OF_PORTS                    			2U					///< Maximum number of ports this library can communicate on
#define NMEA_MAXIMUM_TRANSMIT_MESSAGE_DETAILS           	16U					///< Maximum number of different transmit message types
#define NMEA_MAXIMUM_RECEIVE_MESSAGE_DETAILS           		6U					///< Maximum number of different receive message types
#define NMEA_MAX_MESSAGE_LENGTH     						82U					///< Maximum message length including cr lf
#define NMEA_MIN_MESSAGE_LENGTH     						9U					///< Minumum message length including cr lf
#define NMEA_DPT_DEPTH_PRESENT                				0x00000001UL		///< Message DPT bitfield for depth present
#define NMEA_DPT_DEPTH_OFFSET_PRESENT         				0x00000002UL		///< Message DPT bitfield for offset present
#define NMEA_DPT_DEPTH_MAX_RANGE_PRESENT      				0x00000004UL		///< Message DPT bitfield for depth max range present
#define NMEA_GGA_UTC_PRESENT                  				0x00000001UL		///< Message GGA bitfield for UTC present
#define NMEA_GGA_LATITUDE_PRESENT             				0x00000002UL		///< Message GGA bitfield for latitude present
#define NMEA_GGA_LONGITUDE_PRESENT            				0x00000004UL		///< Message GGA bitfield for longitude present
#define NMEA_GGA_QUALITY_INDICATOR_PRESENT    				0x00000008UL		///< Message GGA bitfield for quality indicator present
#define NMEA_GGA_SATELLITES_IN_USE_PRESENT    				0x00000010UL		///< Message GGA bitfield for satellites in use present
#define NMEA_GGA_HDOP_PRESENT                 				0x00000020UL		///< Message GGA bitfield for hdop present
#define NMEA_GGA_ALTITUDE_PRESENT             				0x00000040UL		///< Message GGA bitfield for altitude present
#define NMEA_GGA_GEIODAL_SEPARATION_PRESENT   				0x00000080UL		///< Message GGA bitfield for geoidal separation present
#define NMEA_GGA_DGPS_AGE_PRESENT             				0x00000100UL		///< Message GGA bitfield for dgps age present
#define NMEA_GGA_DGPS_STATION_ID_PRESENT      				0x00000200UL		///< Message GGA bitfield for dgps station id present
#define NMEA_HDM_MAG_HEADING_PRESENT  						0x00000001UL		///< Message HDM bitfield for magnetic heading present
#define NMEA_HDT_TRUE_HEADING_PRESENT  						0x00000001UL		///< Message HDT bitfield for true heading present
#define NMEA_MTW_WATER_TEMPERATURE_PRESENT 					0x00000001UL		///< Message MTW bitfield for water temperatore present
#define NMEA_MWD_WIND_DIRECTION_TRUE_PRESENT 				0x00000001UL		///< Message MWD bitfield for true wind direction present
#define NMEA_MWD_WIND_DIRECTION_MAG_PRESENT   				0x00000002UL		///< Message MWD bitfield for magnetic wind direction present
#define NMEA_MWD_WIND_SPEED_KTS_PRESENT       				0x00000004UL		///< Message MWD bitfield for wind speed knots present
#define NMEA_MWD_WIND_SPEED_MPS_PRESENT       				0x00000008UL		///< Message MWD bitfield for wind speed m/s present
#define NMEA_MWV_WIND_ANGLE_PRESENT       					0x00000001UL		///< Message MWV bitfield for wind angle present
#define NMEA_MWV_REFERENCE_PRESENT        					0x00000002UL		///< Message MWV bitfield for wind reference present
#define NMEA_MWV_WIND_SPEED_PRESENT       					0x00000004UL		///< Message MWV bitfield for wind speed present
#define NMEA_MWV_WIND_SPEED_UNITS_PRESENT 					0x00000008UL		///< Message MWV bitfield for wind speed units present
#define NMEA_MWV_STATUS_PRESENT           					0x00000010UL		///< Message MWV bitfield for status present
#define NMEA_RMC_UTC_PRESENT            					0x00000001UL		///< Message RMC bitfield for time present
#define NMEA_RMC_STATUS_PRESENT         					0x00000002UL		///< Message RMC bitfield for status present
#define NMEA_RMC_LATITUDE_PRESENT      						0x00000004UL		///< Message RMC bitfield for latitude present
#define NMEA_RMC_LONGITUDE_PRESENT      					0x00000008UL		///< Message RMC bitfield for longitude present
#define NMEA_RMC_SOG_PRESENT            					0x00000010UL		///< Message RMC bitfield for sog present
#define NMEA_RMC_COG_PRESENT            					0x00000020UL		///< Message RMC bitfield for cog present
#define NMEA_RMC_DATE_PRESENT          	 					0x00000040UL		///< Message RMC bitfield for date present
#define NMEA_RMC_MAG_VARIATION_PRESENT  					0x00000080UL		///< Message RMC bitfield for magnetic variation present
#define NMEA_RMC_MAG_DIRECTION_PRESENT  					0x00000100UL		///< Message RMC bitfield for magnetic variation hemisphere present
#define NMEA_RMC_MODE_PRESENT           					0x00000200UL		///< Message RMC bitfield for mode present
#define NMEA_RMC_NAV_STATUS_PRESENT     					0x00000400UL		///< Message RMC bitfield for navigation status present
#define NMEA_VDM_MAX_AIS_DATA_FIELD_LENGTH					62U					///< VDM message data field maximum length
#define NMEA_VDM_FRAGMENT_COUNT_PRESENT  					0x00000001UL		///< Message VDM bitfield for fragment count present
#define NMEA_VDM_FRAGMENT_NUMBER_PRESENT  					0x00000002UL		///< Message VDM bitfield for fragment number present
#define NMEA_VDM_MESSAGE_IDENTIFIER_PRESENT  				0x00000004UL		///< Message VDM bitfield for message identifier present
#define NMEA_VDM_CHANNEL_CODE_PRESENT  						0x00000008UL		///< Message VDM bitfield for channel code present
#define NMEA_VDM_DATA_PRESENT  								0x00000010UL		///< Message VDM bitfield for data present
#define NMEA_VDM_FILL_BITS_PRESENT  						0x00000020UL		///< Message VDM bitfield for fill bits present
#define NMEA_VHW_HEADING_TRUE_PRESENT     					0x00000001UL		///< Message VHW bitfield for heading true present
#define NMEA_VHW_HEADING_MAG_PRESENT      					0x00000002UL		///< Message VHW bitfield for heading magnetic present
#define NMEA_VHW_WATER_SPEED_KTS_PRESENT  					0x00000004UL		///< Message VHW bitfield for water speed knots present
#define NMEA_VHW_WATER_SPEED_KMPH_PRESENT 					0x00000008UL		///< Message VHW bitfield for water speed km/h present
#define NMEA_VLW_TOTAL_WATER_DISTANCE_PRESENT        		0x00000001UL		///< Message VLW bitfield for total water distance present
#define NMEA_VLW_TRIP_WATER_DISTANCE_PRESENT         		0x00000002UL		///< Message VLW bitfield for trip water distance present
#define NMEA_VLW_TOTAL_GROUND_DISTANCE_PRESENT       		0x00000004UL		///< Message VLW bitfield for total ground distance present
#define NMEA_VLW_TRIP_GROUND_DISTANCE_PRESENT        		0x00000008UL		///< Message VLW bitfield for trip ground distance present
#define NMEA_MDA_PRESSURE_INCHES_PRESENT        			0x00000001UL		///< Message MDA bitfield for pressure inches hg present
#define NMEA_MDA_PRESSURE_BARS_PRESENT        				0x00000002UL		///< Message MDA bitfield for pressure bars present
#define NMEA_MDA_AIR_TEMPERATURE_PRESENT        			0x00000004UL		///< Message MDA bitfield for air temperature present
#define NMEA_MDA_WATER_TEMPERATURE_PRESENT        			0x00000008UL		///< Message MDA bitfield for water temperature present
#define NMEA_MDA_RELATIVE_HUMIDITY_PRESENT        			0x000000010UL		///< Message MDA bitfield for relative humidity present
#define NMEA_MDA_ABSOLUTE_HUMIDITY_PRESENT        			0x000000020UL		///< Message MDA bitfield for absolute humidity present
#define NMEA_MDA_DEW_POINT_PRESENT        					0x000000040UL		///< Message MDA bitfield for dew point present
#define NMEA_MDA_WIND_DIRECTION_TRUE_PRESENT        		0x000000080UL		///< Message MDA bitfield for wind direction true present
#define NMEA_MDA_WIND_DIRECTION_MAGNETIC_PRESENT        	0x000000100UL		///< Message MDA bitfield for wind direction magnaetic present
#define NMEA_MDA_WINDSPEED_KNOTS_PRESENT        			0x000000200UL		///< Message MDA bitfield for windpwwd knots present
#define NMEA_MDA_WINDSPEED_MPS_PRESENT        				0x000000400UL		///< Message MDA bitfield for windspeed m/s present
#define NMEA_XDR_MAX_MEASUREMENTS_COUNT            			6U					///< Maximum number of measurements in a single XDR message
#define NMEA_XDR_MEASUREMENT_1_PRESENT                		0x00000001UL		///< Message XDR bitfield for measurement 1 present
#define NMEA_XDR_MEASUREMENT_2_PRESENT                		0x00000002UL		///< Message XDR bitfield for measurement 2 present
#define NMEA_XDR_MEASUREMENT_3_PRESENT                		0x00000004UL		///< Message XDR bitfield for measurement 3 present
#define NMEA_XDR_MEASUREMENT_4_PRESENT                		0x00000008UL		///< Message XDR bitfield for measurement 4 present
#define NMEA_XDR_MEASUREMENT_5_PRESENT                		0x00000010UL		///< Message XDR bitfield for measurement 5 present
#define NMEA_XDR_MEASUREMENT_6_PRESENT                		0x00000020UL		///< Message XDR bitfield for measurement 6 present
#define NMEA_XDR_MAX_ID_LENGTH                      		8U					///< Maximum id length in a XDR message
#define NMEA_SPEED_UP_MESSAGE_PERMIL_PERIOD_ADJUSTMENT		999UL				///< Rate speed up permil value
#define NMEA_SLOW_DOWN_MESSAGE_PERMIL_PERIOD_ADJUSTMENT		1010UL				///< Rate speed down permil value

/************
*** TYPES ***
************/

/**
 * Structure to hold a UTC time.
 */
typedef struct
{
    uint8_t     hours;          ///< Time hours 0-23 */
    uint8_t     minutes;        ///< Time minutes 0-59 */
    float       seconds;        ///< Time seconds 0-59.99 */
} nmea_utc_time_t;

/**
 * Structure to hold a date.
 */
typedef struct
{
    uint8_t date;           ///< date 1 to 31 */
    uint8_t month;          ///< month 1 to 12 */
    uint16_t year;          ///< year 4 digit form */
} nmea_date_t;

/**
 * Enumeration of errors returned by this library
 */
typedef enum
{
    nmea_error_none,		///< No error
	nmea_error_param,		///< An API function call parameter was illegal
    nmea_error_message,		///< A received message was badly formed or a message could not be encoded
	nmea_error_overflow		///< Data overflow when writing to a serial port
} nmea_error_t;

/**
 * Enumeration of all message types recognised by this library
 */
typedef enum
{
    nmea_message_min,           /* must be first value */
    nmea_message_DPT,			///< Depth message
    nmea_message_HDT,			///< Heading true message
    nmea_message_HDM,			///< Heading magnetic
    nmea_message_GGA,			///< GPS message
    nmea_message_MWD,			///< Wind data ground referenced message
    nmea_message_MWV,			///< Wind date boat referenced message
    nmea_message_MTW,			///< Water temperature message
    nmea_message_RMC,			///< GPS message
    nmea_message_VDM,			///< AIS message
    nmea_message_VHW,			///< Heading and boat speed message
    nmea_message_VLW,			///< Distances message
    nmea_message_XDR,			///< Transducer message
	nmea_message_MDA,			///< Envirnment message
    nmea_message_max            /* must be last value */
} nmea_message_type_t;

/**
 * Typedef of callback function called when a message has been received
 *
 * @param message The message received
 */
typedef void (*nmea_receive_message_callback_t)(const char *message);

/**
 * Typedef of callback function called when a message is to be transmitted to get the data that needs to be transmitted
 */
typedef void (*nmea_get_transmit_data_callback_t)(void);

/**
 * Typedef of callback function called to encode a message
 *
 * @param message_data Buffer to hold encoded message
 * @param source Pointer to structure holding data to be encoded
 * @return One of the error codes defined above
 */
typedef nmea_error_t (*nmea_encoder_function_t)(const char *message_data, void *source);

/**
 * Structure holding data on each message type to be transmitted per port
 */
typedef struct
{
    nmea_message_type_t message_type;									///< The message type
    uint8_t port;														///< The port to transmit the message on
    uint32_t transmit_period_ms;										///< The transmit period
    nmea_get_transmit_data_callback_t get_transmit_data_callback;		///< Callback function to get the data to transmit
    void *get_data_structure;											///< Structure holding the data to be transmitted
    nmea_encoder_function_t encoder;									///< Callback function to encode the message
} transmit_message_details_t;

/**
 * Structure holding data on each message type to be received per port
 */
typedef struct
{
    nmea_message_type_t message_type;									///< The message type
    uint8_t port;														///< The port to receive the message on
    nmea_receive_message_callback_t receive_message_callback;			///< Callback function to be called when a message of this type has been received on this port
} nmea_receive_message_details_t;

/**
 * Structure for message data for message type DPT
 */
typedef struct
{
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    float depth;					///< Depth in m
    float depth_offset;				///< Trnasducer offset in m
    float depth_maximum_range;		///< Depth maximum range
} nmea_message_data_DPT_t;

/**
 * Structure for message data for message type GGA
 */
typedef struct
{
    uint8_t quality_indicator;		///< Signal quality indicator - see NMEA0183 manual 
    uint8_t satellites_in_use;		///< Number of satellites in use to make calculation
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    nmea_utc_time_t utc;			///< Time in UTC
    uint16_t dgps_station_id;		///< DGPS station id
    float latitude;					///< Latitude, for format see NMEA0183 manual
    float longitude;				///< Longitude, for format see NMEA0183 manual
    float HDOP;						///< Horizontal Dilution of Position
    float altitude;					///< Altitude above geoid in m
    float geoidal_separation;		///< Geoidal separation
    float dgpsAge;					///< DGPS age in s
} nmea_message_data_GGA_t;

/**
 * Structure for message data for message type HDM
 */
typedef struct
{
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    float magnetic_heading;			///< Heading magnetic
} nmea_message_data_HDM_t;

/**
 * Structure for message data for message type HDT
 */
typedef struct
{
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    float true_heading;				///< Heading true
} nmea_message_data_HDT_t;

/**
 * Structure for message data for message type MTW
 */
typedef struct
{
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    float water_temperature;		///< Sea water temperature
} nmea_message_data_MTW_t;

/**
 * Structure for message data for message type MWD
 */
typedef struct
{
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    float wind_direction_true;		///< Wind direction true ground referenced
    float wind_direction_magnetic;	///< Wind direction magnetic ground referenced
    float wind_speed_knots;			///< Wind speed ground referenced knots
    float wind_speed_mps;			///< Wind speed ground referenced m/s
} nmea_message_data_MWD_t;

/**
 * Structure for message data for message type MWV
 */
typedef struct
{
    char wind_speed_units;			///< Windspeed units - see NMEA0183 manual
    char status;					///< Status - see NMEA0183 manual
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    float wind_angle;				///< Wind angle degrees
    float wind_speed;				///< Wind speed knots
    char reference;					///< What the wind measurements are referenced to - see NMEA0183 manual
} nmea_message_data_MWV_t;

/**
 * Structure for message data for message type RMC
 */
typedef struct
{
    char magnetic_variation_direction;	///< Magnetic variation direction, E or W
    char status;					///< Signal status - see NMEA0183 manual
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    nmea_utc_time_t utc;			///< UTC time
    char mode;						///< Reception mode - see NMEA0183 manual
    char navigation_status;			///< Navigation status - see NMEA0183 manual
    float latitude;					///< Latitude, for format see NMEA0183 manual
    float longitude;                ///< Longitude, for format see NMEA0183 manual
    float SOG;						///< Speed over ground knots
    float COG;						///< Course over ground degrees
    nmea_date_t date;				///< Date
    float magnetic_variation;		///< Magnetic variation degrees
} nmea_message_data_RMC_t;

/**
 * Structure for message data for message type VDM
 */
typedef struct
{
    char channel_code;				///< AIS channel 
    uint8_t fill_bits;				///< AIS fill bits
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    uint8_t fragment_count;			///< Number of fragments to complete message
    uint8_t fragment_number;		///< This fragment count
    uint8_t message_identifier;		///< Message identifier
    char data[NMEA_VDM_MAX_AIS_DATA_FIELD_LENGTH + 1];		///< AIS encoded data
} nmea_message_data_VDM_t;

/**
 * Structure for message data for message type VHW
 */
typedef struct
{
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    float heading_true;				///< Boat heading true
    float heading_magnetic;			///< Boat heading magnetic
    float water_speed_knots;		///< Speed through water knots
    float water_speed_kmph;			///< Speed through water km/h
} nmea_message_data_VHW_t;

/**
 * Structure for message data for message type MDA
 */
typedef struct
{
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    float pressure_inches;			///< Atmospheric pressure inches hg
    float pressure_bars;			///< Atmospheric pressure bar
    float air_temperature;			///< Air temperature degrees C
    float water_temperature;		///< Sea water temperature degrees C
    float relative_huimidity;		///< Relative humidity
    float absolute_humidity;		///< Absolute humidity 
    float dew_point;				///< Dew point degrees C
    float wind_direction_true;		///< Wind direction true ground referenced 
    float wind_direction_magnetic;  ///< Wind direction magnetic ground referenced
    float windspeed_knots;			///< Wind speed ground referenced knots
    float windspeed_mps;            ///< Wind speed ground referenced m/s
} nmea_message_data_MDA_t;

/**
 * Structure for message data for message type VLW
 */
typedef struct
{
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    float total_water_distance;		///< Total distance water referenced
    float trip_water_distance;		///< Trip distance water referenced
    float total_ground_distance;	///< Total distance ground referenced
    float trip_ground_distance;     ///< Trip distance ground referenced
} nmea_message_data_VLW_t;

/**
 * Structure for a single reading in message type XDR
 */
typedef struct
{
    char transducer_type;			///< Transducer type - see NMEA0183 manual
    char transducer_id[NMEA_XDR_MAX_ID_LENGTH+1];	///< Unique identifer for this transducer
    char units;						///< Units -  - see NMEA0183 manual
    uint8_t decimal_places;			///< Decimal places to reading
    float measurement;				///< The transducer reading
} nmea_XDR_tuple_t;

/**
 * Structure for message data for message type XDR
 */
typedef struct
{
    uint32_t data_available;		///< Bitfield of what fields are present in the message
    nmea_XDR_tuple_t measurements[NMEA_XDR_MAX_MEASUREMENTS_COUNT];		///< Array of tuples of transducer readings
} nmea_message_data_XDR_t;

/*************************
*** EXTERNAL VARIABLES ***
*************************/

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Enable the transmission of a specific message type on a specific port
 *
 * @param nmea_transmit_message_details Details of message type, port, period
 */
void nmea_enable_transmit_message(const transmit_message_details_t *nmea_transmit_message_details);

/**
 * Disable the transmission of a specific message type on a specific port
 *
 * @param port The port to disable this type transmission
 * @param message_type The message type
 */
void nmea_disable_transmit_message(uint8_t port, nmea_message_type_t message_type);

/**
 * Enable the reception of a specific message type on a specific port
 *
 * @param nmea_receive_message_details Details of message type, port, period
 */
void nmea_enable_receive_message(const nmea_receive_message_details_t *nmea_receive_message_details);

/**
 * Disable the transmission of a specific message type on a specific port
 *
 * @param port The port to disable this type transmission
 * @param message_type The message type
 */
void nmea_transmit_message_now(uint8_t port, nmea_message_type_t message_type);

/**
 * Main library processing function
 *
 * @note Call this periodically. Nothing happens unless this is done
 */
void nmea_process(void);

/**
 * Decode a GGA message
 *
 * @param message_data The received message
 * @param result Structure to contain the decoded values
 * @return Error code from above enum
 */
nmea_error_t nmea_decode_GGA(const char *message_data, nmea_message_data_GGA_t *result);

/**
 * Encode a GGA message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_GGA(char *message_data,  const void *source);

/**
 * Encode a HDM message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_HDM(char *message_data, const void *source);

/**
 * Encode a MTW message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_MTW(char *message_data, const void *source);

/**
 * Encode a DPT message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_DPT(char *message_data, const void *source);

/**
 * Encode a MWD message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_MWD(char *message_data, const void *source);

/**
 * Encode a MWV message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_MWV(char *message_data, const void *source);

/**
 * Decode a RMC message
 *
 * @param message_data The received message
 * @param result Structure to contain the decoded values
 * @return Error code from above enum
 */
nmea_error_t nmea_decode_RMC(const char *message_data, nmea_message_data_RMC_t *result);

/**
 * Encode a RMC message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_RMC(char *message_data, const void *source);

/**
 * Decode a VDM message
 *
 * @param message_data The received message
 * @param result Structure to contain the decoded values
 * @return Error code from above enum
 */
nmea_error_t nmea_decode_VDM(const char *message_data, nmea_message_data_VDM_t *result);

/**
 * Encode a VDM message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_VDM(char *message_data, const void *source);

/**
 * Encode a VLW message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_VLW(char *message_data, const void *source);

/**
 * Encode a VHW message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_VHW(char *message_data,  const void *source);

/**
 * Encode a XDR message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_XDR(char *message_data, const void *source);

/**
 * Encode a HDT message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_HDT(char *message_data, const void *source);

/**
 * Encode a MDA message
 *
 * @param message_data The encoded message
 * @param source The source of the value to encode that is cast to a data specific type
 * @return Error code from above enum
 */
nmea_error_t nmea_encode_MDA(char *message_data, const void *source);

#ifdef __cplusplus
}
#endif

#endif
