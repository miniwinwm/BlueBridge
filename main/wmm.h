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

#ifndef WMM_H
#define WMM_H

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

#define WMM_EPOCH		2020.0f			///< the start year of the model coefficients

/************
*** TYPES ***
************/

/**
 * A field in the model coefficients table 
 */
typedef struct
{
	float gnm;
	float hnm;
	float dgnm;
	float dhnm;
} wmm_cof_record_t;

/*************************
*** EXTERNAL VARIABLES ***
*************************/

extern const uint8_t wmm_cof_entries_encoded[];		///< the encoded coefficients table

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Get magnetic declination for a time and location 
 *
 * @param glat Latitude in degrees and fraction degrees, negative for south, 0-90
 * @param glon Longitude in degrees and fraction degrees, negative for west, -180 to 180
 * @param time_years Time in years and fractional years
 * @param dec Pointer to float for result, declination in degrees, west negative
 */
void E0000(float glat, float glon, float time_years, float *dec);

/**
 * Initialize the model. Call once before calling other functions
 */
void wmm_init(void);

/**
 * Get the date in the years and fractional years
 *
 * @param year 2 digit format
 * @param month 1-12
 * @param date 1-31
 * @return Date in years and fractional years
 */
float wmm_get_date(uint8_t year, uint8_t month, uint8_t date);

#ifdef __cplusplus
}
#endif

#endif
