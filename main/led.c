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

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "led.h"

/**************
*** DEFINES ***
**************/

/************
*** TYPES ***
************/

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

static void led_timer_callback(xTimerHandle pxTimer);

/**********************
*** LOCAL VARIABLES ***
**********************/

static xTimerHandle timer_led;
static uint32_t current_period;

/***********************
*** GLOBAL VARIABLES ***
***********************/

/****************
*** CONSTANTS ***
****************/

/**********************
*** LOCAL FUNCTIONS ***
**********************/

static void led_timer_callback(xTimerHandle pxTimer)
{
	gpio_set_level(GPIO_NUM_17, 0UL);	
	current_period = 0UL;
}

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void led_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << GPIO_NUM_17;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);	
	
	gpio_set_level(GPIO_NUM_17, 0UL);
	
	timer_led = xTimerCreate("timerled", pdMS_TO_TICKS(1000), pdFALSE, (void*)0, led_timer_callback); 
}

void led_flash(uint32_t ms)
{
	if (timer_led != NULL && ms > 0UL)
	{
		if (ms > current_period)
		{
			current_period = ms;
		
			(void)xTimerStop(timer_led, (TickType_t)0);
			(void)xTimerChangePeriod(timer_led, pdMS_TO_TICKS(ms), (TickType_t)0);		
			(void)xTimerStart(timer_led, (TickType_t)0);
			gpio_set_level(GPIO_NUM_17, 1UL);		
		}			
	}
}