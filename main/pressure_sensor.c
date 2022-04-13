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

/***************
*** INCLUDES ***
***************/

#include <stdbool.h>
#include <string.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "pressure_sensor.h"
#include "main.h"
#include "esp_log.h"

/************
*** TYPES ***
************/

typedef struct
{
	uint8_t register_address;
	uint8_t *coefficient;
} coefficients_table_entry;

/**********************
*** LOCAL VARIABLES ***
**********************/

static uint16_t dig_T1;
static int16_t dig_T2;
static int16_t dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2;
static int16_t dig_P3;
static int16_t dig_P4;
static int16_t dig_P5;
static int16_t dig_P6;
static int16_t dig_P7;
static int16_t dig_P8;
static int16_t dig_P9;
static int32_t t_fine;
static QueueHandle_t pressure_sensor_queue_handle;

/****************
*** CONSTANTS ***
****************/

#define I2C_PRESSURE_SENSOR_ADDRESS			0x76U
#define I2C_MEASUREMENT_START_WAIT_MS       500
#define I2C_MEASUREMENT_PERIOD_MS           1000U
#define I2C_TIMEOUT_MS                      1000U

static const coefficients_table_entry coefficients_table[] = {{0x88U, (uint8_t *)&dig_T1},
		{0x8aU, (uint8_t *)&dig_T2},
		{0x8cU, (uint8_t *)&dig_T3},
		{0x8eU, (uint8_t *)&dig_P1},
		{0x90U, (uint8_t *)&dig_P2},
		{0x92U, (uint8_t *)&dig_P3},
		{0x94U, (uint8_t *)&dig_P4},
		{0x96U, (uint8_t *)&dig_P5},
		{0x98U, (uint8_t *)&dig_P6},
		{0x9aU, (uint8_t *)&dig_P7},
		{0x9cU, (uint8_t *)&dig_P8},
		{0x9eU, (uint8_t *)&dig_P9}
};

/***********************
*** GLOBAL VARIABLES ***
***********************/

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

static void bmp280_compensate_T_int32(int32_t adc_T);
static uint32_t bmp280_compensate_P_int64(int32_t adc_P);
static bool i2c_send(uint8_t address, uint8_t reg, uint8_t data);
static bool i2c_receive(uint8_t address, uint8_t reg, uint8_t *read_value);
static bool i2c_receive_multi(uint8_t address, uint8_t reg, uint8_t *read_value, uint8_t length);

/**********************
*** LOCAL FUNCTIONS ***
**********************/

static bool i2c_send(uint8_t address, uint8_t reg, uint8_t data)
{
    uint8_t write_buffer[2];
    bool result_ok = true;
        
    write_buffer[0] = reg;
    write_buffer[1] = data;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    (void)i2c_master_start(cmd);
    (void)i2c_master_write_byte(cmd, address << 1, true);
    (void)i2c_master_write(cmd, write_buffer, (size_t)2, true);
    (void)i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(I2C_NUM_0, cmd, I2C_TIMEOUT_MS / portTICK_RATE_MS) != ESP_OK)
    {
        result_ok = false;
    }    
    i2c_cmd_link_delete(cmd);

    return result_ok;
}

static bool i2c_receive_multi(uint8_t address, uint8_t reg, uint8_t *read_value, uint8_t length)
{
	uint8_t i;
	bool result = true;

	for (i = 0U; i < length; i++)
	{
		result = result && i2c_receive(address, reg + i, read_value + i);
	}

	return result;
}

static bool i2c_receive(uint8_t address, uint8_t reg, uint8_t *read_value)
{   
    bool result_ok = true;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    (void)i2c_master_start(cmd);
    (void)i2c_master_write_byte(cmd, address << 1, true);
    (void)i2c_master_write_byte(cmd, reg, true);    
    (void)i2c_master_start(cmd);   
    (void)i2c_master_write_byte(cmd, (address << 1) | 0x01, true);     
    (void)i2c_master_read_byte(cmd, read_value, I2C_MASTER_NACK);
    (void)i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(I2C_NUM_0, cmd, I2C_TIMEOUT_MS / portTICK_RATE_MS) != ESP_OK)
    {
        result_ok = false;
    }
    
    i2c_cmd_link_delete(cmd);

	return result_ok;
}

static void bmp280_compensate_T_int32(int32_t adc_T)
{
	int32_t var1;
	int64_t var2;

	var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;
}

static uint32_t bmp280_compensate_P_int64(int32_t adc_P)
{
	int64_t varl;
	int64_t var2;
	int64_t p;

	varl = ((int64_t)t_fine) - 128000LL;
	var2 = varl * varl * (int64_t)dig_P6;
	var2 = var2 + ((varl * (int64_t)dig_P5) << 17);
	var2 = var2 + (((int64_t)dig_P4) << 35);
	varl = ((varl * varl * (int64_t)dig_P3) >> 8) + ((varl * (int64_t)dig_P2) << 12);
	varl = (((((int64_t)1LL) << 47) + varl)) * ((int64_t)dig_P1) >> 33;
	if (varl == 0LL)
	{
		return 0UL; // avoid exception caused by division by zero
	}

	p = 1048576LL - (int64_t)adc_P;
	p = (((p << 31) - var2) * 3125LL) / varl;
	varl = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((int64_t)dig_P8) * p) >> 19;
	p = ((p + varl + var2) >> 8) + (((int64_t)dig_P7) << 4);

	return (uint32_t)p;
}

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void pressure_sensor_init(void)
{
    i2c_config_t conf = 
    {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 18,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = 19,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
        .clk_flags = 0
    };
    
    (void)i2c_param_config(I2C_NUM_0, &conf);
    (void)i2c_driver_install(I2C_NUM_0, conf.mode, (size_t)0, (size_t)0, 0);
}

bool pressure_sensor_start_measurement_mb(void)
{
    return i2c_send(I2C_PRESSURE_SENSOR_ADDRESS, 0xf4U, 0x25U);
}

bool pressure_sensor_get_measurement_mb(float *read_measurement)
{
    int32_t adc_T;
    int32_t adc_P;
    uint8_t msb = 0U;
    uint8_t lsb = 0U;
    uint8_t xlsb = 0U;
    uint32_t P;

    if (!i2c_receive(I2C_PRESSURE_SENSOR_ADDRESS, 0xfaU, &msb))
    {
    	return false;
    }
    if (!i2c_receive(I2C_PRESSURE_SENSOR_ADDRESS, 0xfbU, &lsb))
    {
    	return false;
    }
    if (!i2c_receive(I2C_PRESSURE_SENSOR_ADDRESS, 0xfcU, &xlsb))
    {
    	return false;
    }

    adc_T = (uint32_t)(xlsb >> 4);
    adc_T |= ((uint32_t)lsb) << 4;
    adc_T |= ((uint32_t)msb) << 12;

    bmp280_compensate_T_int32((int32_t)adc_T);

    if (!i2c_receive(I2C_PRESSURE_SENSOR_ADDRESS, 0xf7U, &msb))
	{
		return false;
	}
    if (!i2c_receive(I2C_PRESSURE_SENSOR_ADDRESS, 0xf8U, &lsb))
	{
		return false;
	}
    if (!i2c_receive(I2C_PRESSURE_SENSOR_ADDRESS, 0xf9U, &xlsb))
	{
		return false;
	}

    adc_P = (uint32_t)(xlsb >> 4);
    adc_P |= ((uint32_t)lsb) << 4;
    adc_P |= ((uint32_t)msb) << 12;

    P = bmp280_compensate_P_int64((int32_t)adc_P);
    *read_measurement = (float)P / 25600.0f;

    return true;
}

void pressure_sensor_task(void *parameters)
{
	bool pressure_sensor_init_ok;
	float latest_pressure_reading;
	uint8_t i;
    
    ESP_LOGI(pcTaskGetName(NULL), "pressure sensor task started");

	(void)memcpy(&pressure_sensor_queue_handle, parameters, sizeof(QueueHandle_t));

    // read calibration data from pressure sensor
	pressure_sensor_init_ok = true;
	for (i = 0U; i < sizeof(coefficients_table) / sizeof(coefficients_table_entry); i++)
	{
		pressure_sensor_init_ok = pressure_sensor_init_ok && i2c_receive_multi(I2C_PRESSURE_SENSOR_ADDRESS,
				coefficients_table[i].register_address,
				coefficients_table[i].coefficient,
				2U);
	}

	// signal main task that this task has started
	(void)xTaskNotifyGive(get_main_task_handle());

    while (true)
    {
		if (pressure_sensor_init_ok && pressure_sensor_start_measurement_mb())
		{
			vTaskDelay((TickType_t)I2C_MEASUREMENT_START_WAIT_MS / portTICK_RATE_MS);
 
			if (pressure_sensor_get_measurement_mb(&latest_pressure_reading))
			{                      
				if (latest_pressure_reading > 920UL && latest_pressure_reading < 1080UL)
				{
					(void)xQueueSend(pressure_sensor_queue_handle, (const void *)&latest_pressure_reading, (TickType_t)100 / portTICK_RATE_MS);
				}
			}
		}
        
		vTaskDelay((TickType_t)I2C_MEASUREMENT_PERIOD_MS / portTICK_RATE_MS);
    }
}
