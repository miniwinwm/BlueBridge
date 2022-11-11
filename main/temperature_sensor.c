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

/***************
*** INCLUDES ***
***************/

#include <math.h>
#include <stdbool.h>
#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "temperature_sensor.h"
#include "esp_log.h"

/**************
*** DEFINES ***
**************/

#define VOLTAGE_DIVIDER_RESISTANCE    	9310.0f        
#define NO_OF_SAMPLES   				32
#define VOLTAGE_REF						5.0f       

/************
*** TYPES ***
************/

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

/**
 * Convert a sensor resistance to a sensor temperature for a PT-1000 sensor
 *
 * @param r resistance in ohms
 * @return temperature in degrees C
 */
static float temperature_sensor_temp_from_resistance(float r);

/**********************
*** LOCAL VARIABLES ***
**********************/

static esp_adc_cal_characteristics_t adc_chars;
static float port_temperatures[5];
static float starboard_temperatures[5];
static uint8_t next_port_temperature_position;
static uint8_t next_starboard_temperature_position;

/***********************
*** GLOBAL VARIABLES ***
***********************/

/****************
*** CONSTANTS ***
****************/

/**********************
*** LOCAL FUNCTIONS ***
**********************/

static float temperature_sensor_temp_from_resistance(float r)
{
    float t;
    const float A  =  3.90802e-3f;
    const float B  = -5.80195e-7f;
    const float R0 =  1000.0f;

    t = A * A - 4.00f * B * (1.0f - r / R0);
   
    if (t < 0.0f)
    {
        // trap sqrt error
        return -9999.99f;
    }
   
    t = (-A + sqrt(t)) / (2.0f * B);
   
    return t;
}

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void temperature_sensor_init(void)
{
	if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) 
	{
        ESP_LOGI(pcTaskGetName(NULL), "eFuse Vref: Supported");
    } 
	else 
	{
        ESP_LOGI(pcTaskGetName(NULL), "eFuse Vref: NOT supported");
    }

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_0);
	adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_0);	
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 0, &adc_chars);	
	
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) 
	{
        ESP_LOGI(pcTaskGetName(NULL), "Characterized using Two Point Value");
    } 
	else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) 
	{
        ESP_LOGI(pcTaskGetName(NULL), "Characterized using eFuse Vref");
    } 
	else 
	{
        ESP_LOGI(pcTaskGetName(NULL), "Characterized using Default Vref");
    }	
}

float temperature_sensor_read_port(void)
{
	uint32_t adc_readings[NO_OF_SAMPLES];
 	bool sorted;
	uint32_t swap_value;
	uint32_t adc_reading;
	
	for (int i = 0; i < NO_OF_SAMPLES; i++) 
	{
		adc_readings[i] = adc1_get_raw(ADC1_CHANNEL_4);
	}

	do
	{
		sorted = true;
		for (int i = 0U; i < (NO_OF_SAMPLES - 1U); i++)
		{
			if (adc_readings[i] > adc_readings[i + 1U])
			{
				swap_value = adc_readings[i + 1U];
				adc_readings[i + 1U] = adc_readings[i];
				adc_readings[i] = swap_value;
				sorted = false;
			}
		}
	}
	while (!sorted);	
	adc_reading = (adc_readings[NO_OF_SAMPLES / 2] + adc_readings[NO_OF_SAMPLES / 2 - 1]) / 2;
	
	// convert adc_reading to voltage in mV
	uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
	float voltagef = ((float)voltage) / 1000.0f;
	ESP_LOGI(pcTaskGetName(NULL), "Port temperature sensor voltage: %f", voltagef);
	float resistance = (VOLTAGE_DIVIDER_RESISTANCE * voltagef) / (VOLTAGE_REF - voltagef);
	
	port_temperatures[next_port_temperature_position] = temperature_sensor_temp_from_resistance(resistance);
	next_port_temperature_position++;
	if (next_port_temperature_position == sizeof(port_temperatures) / sizeof(float))
	{
		next_port_temperature_position = 0U;
	}
	
	float mean_temperature = 0.0f;
	for (uint8_t i = 0U; i < sizeof(port_temperatures) / sizeof(float); i++)
	{
		mean_temperature += port_temperatures[i];
	}
	
	return mean_temperature / (float)(sizeof(port_temperatures) / sizeof(float));
}

float temperature_sensor_read_starboard(void)
{
	uint32_t adc_readings[NO_OF_SAMPLES];
 	bool sorted;
	uint32_t swap_value;
	uint32_t adc_reading;
	
	for (int i = 0; i < NO_OF_SAMPLES; i++) 
	{
		adc_readings[i] = adc1_get_raw(ADC1_CHANNEL_5);
	}

	do
	{
		sorted = true;
		for (int i = 0U; i < (NO_OF_SAMPLES - 1U); i++)
		{
			if (adc_readings[i] > adc_readings[i + 1U])
			{
				swap_value = adc_readings[i + 1U];
				adc_readings[i + 1U] = adc_readings[i];
				adc_readings[i] = swap_value;
				sorted = false;
			}
		}
	}
	while (!sorted);	
	adc_reading = (adc_readings[NO_OF_SAMPLES / 2] + adc_readings[NO_OF_SAMPLES / 2 - 1]) / 2;
	
	// convert adc_reading to voltage in mV
	uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
	float voltagef = ((float)voltage) / 1000.0f;
	ESP_LOGI(pcTaskGetName(NULL), "Starboard temperature sensor voltage: %f", voltagef);
	float resistance = (VOLTAGE_DIVIDER_RESISTANCE * voltagef) / (VOLTAGE_REF - voltagef);
	
	next_starboard_temperature_position++;
	if (next_starboard_temperature_position == sizeof(starboard_temperatures) / sizeof(float))
	{
		next_starboard_temperature_position = 0U;
	}
	
	float mean_temperature = 0.0f;
	for (uint8_t i = 0U; i < sizeof(starboard_temperatures) / sizeof(float); i++)
	{
		mean_temperature += starboard_temperatures[i];
	}	
	
	return temperature_sensor_temp_from_resistance(resistance);
}
