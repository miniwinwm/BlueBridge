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

This work is based on code in file BluetoothSerial.cpp distributed as part of 
the ESP32 driver layer of the Arduino library for the ESP32 platform. The 
license for the source code is given below:

// Copyright 2018 Evandro Luis Copercini
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License. 

The above source code has been modified to enable it to build directly 
with the ESP32-IDF framework.

*/

/***************
*** INCLUDES ***
***************/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "spp_acceptor.h"

/**************
*** DEFINES ***
**************/

#define SPP_SERVER_NAME 			"SPP_SERVER"		///< BlueDroid bluetooth serial server name, not apparent externally
#define DEVICE_NAME 				"BlueBridge"		///< Bluetooth device name as seen by remote device
#define RX_QUEUE_SIZE 				512					///< Queue size for incoming data in bytes
#define TX_QUEUE_SIZE 				32					///< Queue size for outgoing data in packets
#define SPP_NOT_CONGESTED   		0x04				///< Event group for when transmit is not busy
#define SPP_TX_QUEUE_TIMEOUT 		1000				///< Transmit timeout for space to become available on transmit queue in OS ticks
#define SPP_TX_DONE_TIMEOUT 		1000				///< Transmit timeout for completion in OS ticks
#define SPP_NOT_CONGESTED_TIMEOUT 	1000				///< Transmit timeout for congestion to clear in OS ticks
#define SPP_TX_MAX 					330					///< Transmit buffer size

/************
*** TYPES ***
************/

/**
 * Transmit packet structure
 */
typedef struct
{
	size_t len;				///< Length of data in bytes
	uint8_t data[];			///< The data to transmit
} spp_packet_t;

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

static bool spp_send_buffer();
static void spp_tx_task(void *arg);
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

/**********************
*** LOCAL VARIABLES ***
**********************/

static xQueueHandle spp_rx_queue;				///< Receive queue OS object
static xQueueHandle spp_tx_queue;				///< Transmit queue OS object
static EventGroupHandle_t spp_event_group;		///< OS object used for flow control 
static SemaphoreHandle_t spp_tx_done;			///< Sempahore released when transmission completed
static TaskHandle_t spp_task_handle;			///< OS handle of the task that this code runs in
static uint32_t spp_client;						///< Handle of client of SPP library object
static uint8_t spp_tx_buffer[SPP_TX_MAX];		///< Transmit buffer
static uint16_t spp_tx_buffer_len = 0;			///< Current data length in transmit buffer

/***********************
*** GLOBAL VARIABLES ***
***********************/

/****************
*** CONSTANTS ***
****************/

/**********************
*** LOCAL FUNCTIONS ***
**********************/

/**
 * Send the contents of the transmit buffer
 *
 * @return If sent ok true else false
 */
static bool spp_send_buffer()
{
    if ((xEventGroupWaitBits(spp_event_group, SPP_NOT_CONGESTED, pdFALSE, pdTRUE, SPP_NOT_CONGESTED_TIMEOUT) & SPP_NOT_CONGESTED) != 0)
    {
        if(!spp_client)
        {
            return false;
        }

        esp_err_t err = esp_spp_write(spp_client, spp_tx_buffer_len, spp_tx_buffer);

        if (err != ESP_OK)
        {
            return false;
        }

        spp_tx_buffer_len = 0;
		if (xSemaphoreTake(spp_tx_done, SPP_TX_DONE_TIMEOUT) != pdTRUE)
		{
            return false;
        }
        return true;
    }
	
    return false;
}

/**
 * Task function running the bluetooth code
 * @param arg Unused
 */
static void spp_tx_task(void *arg)
{
    spp_packet_t *packet = NULL;
    size_t len = (size_t)0;
	size_t to_send = (size_t)0;
    uint8_t *data = NULL;

    while (true)
    {
        if (spp_tx_queue && xQueueReceive(spp_tx_queue, &packet, portMAX_DELAY) == pdTRUE && packet)
        {
            if (packet->len <= (SPP_TX_MAX - spp_tx_buffer_len))
            {
                (void)memcpy(spp_tx_buffer + spp_tx_buffer_len, packet->data, packet->len);
                spp_tx_buffer_len += packet->len;
                vPortFree(packet);
                packet = NULL;
                if (SPP_TX_MAX == spp_tx_buffer_len || uxQueueMessagesWaiting(spp_tx_queue) == 0)
                {
                    spp_send_buffer();
                }
            }
            else
            {
                len = packet->len;
                data = packet->data;
                to_send = SPP_TX_MAX - spp_tx_buffer_len;
                (void)memcpy(spp_tx_buffer + spp_tx_buffer_len, data, to_send);
                spp_tx_buffer_len = SPP_TX_MAX;
                data += to_send;
                len -= to_send;
                if (!spp_send_buffer())
                {
                    len = 0;
                }
                while (len >= SPP_TX_MAX)
                {
                    (void)memcpy(spp_tx_buffer, data, SPP_TX_MAX);
                    spp_tx_buffer_len = SPP_TX_MAX;
                    data += SPP_TX_MAX;
                    len -= SPP_TX_MAX;
                    if (!spp_send_buffer())
                    {
                        len = 0;
                        break;
                    }
                }
                if (len)
                {
                    (void)memcpy(spp_tx_buffer, data, len);
                    spp_tx_buffer_len += len;
                    if (uxQueueMessagesWaiting(spp_tx_queue) == 0)
                    {
                        spp_send_buffer();
                    }
                }
                vPortFree(packet);
                packet = NULL;
            }
        }
    }
}

/**
 * Callback function called from the bluetooth stack when an event has happened
 *
 * @param event The event that has happened
 * @param param Data that corresponds to the event
 */
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event)
    {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_INIT_EVT");
        esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME);
        break;

    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_DISCOVERY_COMP_EVT");
        break;

    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_OPEN_EVT");
        if (!spp_client)
        {
        	spp_client = param->open.handle;
        }
        xEventGroupSetBits(spp_event_group, SPP_NOT_CONGESTED);
        break;

    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_CLOSE_EVT");
        if ((param->close.async == false && param->close.status == ESP_SPP_SUCCESS) || param->close.async)
        {
			spp_client = 0;
			xEventGroupSetBits(spp_event_group, SPP_NOT_CONGESTED);
		}
        break;

    case ESP_SPP_START_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_START_EVT");
        esp_bt_dev_set_device_name(DEVICE_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;

    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_CL_INIT_EVT");
        break;

    case ESP_SPP_DATA_IND_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_DATA_IND_EVT len=%d handle=%d", param->data_ind.len, param->data_ind.handle);
        for (int i = 0; i < param->data_ind.len; i++)
        {
        	if (xQueueSend(spp_rx_queue, param->data_ind.data + i, (TickType_t)0) != pdTRUE)
        	{
				ESP_LOGI(pcTaskGetName(NULL), "RX Full! Discarding %u bytes", param->data_ind.len - i);
				break;
			}
        }
        break;

    case ESP_SPP_CONG_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_CONG_EVT");
        if (param->cong.cong)
        {
            xEventGroupClearBits(spp_event_group, SPP_NOT_CONGESTED);
        }
        else
        {
            xEventGroupSetBits(spp_event_group, SPP_NOT_CONGESTED);
        }
        break;

    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_WRITE_EVT");
        if (param->write.status == ESP_SPP_SUCCESS)
        {
            if (param->write.cong)
            {
                xEventGroupClearBits(spp_event_group, SPP_NOT_CONGESTED);
            }
        }
        xSemaphoreGive(spp_tx_done);
        break;

    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_SRV_OPEN_EVT");
        if (param->srv_open.status == ESP_SPP_SUCCESS)
        {
        	spp_client = param->srv_open.handle;
			spp_tx_buffer_len = 0;
        }
        break;

    case ESP_SPP_SRV_STOP_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_SRV_STOP_EVT");
        break;

    case ESP_SPP_UNINIT_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_SPP_UNINIT_EVT");
        break;

    default:
        break;
    }
}

/**
 * Callback function called from the bluetooth stack when a General Access Protocol event has happened
 *
 * @param event The event that has happened
 * @param param Data that corresponds to the event
 */
static void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
		{
			if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
			{
				ESP_LOGI(pcTaskGetName(NULL), "authentication success: %s", param->auth_cmpl.device_name);
				esp_log_buffer_hex(pcTaskGetName(NULL), param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
			}
			else
			{
				ESP_LOGE(pcTaskGetName(NULL), "authentication failed, status:%d", param->auth_cmpl.stat);
			}
			break;
		}

    case ESP_BT_GAP_PIN_REQ_EVT:
		{
			ESP_LOGI(pcTaskGetName(NULL), "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
			if (param->pin_req.min_16_digit)
			{
				ESP_LOGI(pcTaskGetName(NULL), "Input pin code: 0000 0000 0000 0000");
				esp_bt_pin_code_t pin_code = {0};
				esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
			}
			else
			{
				ESP_LOGI(pcTaskGetName(NULL), "Input pin code: 1234");
				esp_bt_pin_code_t pin_code;
				pin_code[0] = '1';
				pin_code[1] = '2';
				pin_code[2] = '3';
				pin_code[3] = '4';
				esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
			}
			break;
		}

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(pcTaskGetName(NULL), "ESP_BT_GAP_MODE_CHG_EVT mode:%d", param->mode_chg.mode);
        break;

    default:
		{
			ESP_LOGI(pcTaskGetName(NULL), "event: %d", event);
			break;
		}
    }
    return;
}

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void spp_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    spp_event_group = xEventGroupCreate();
    xEventGroupClearBits(spp_event_group, 0xFFFFFF);
    xEventGroupSetBits(spp_event_group, SPP_NOT_CONGESTED);
    spp_rx_queue = xQueueCreate(RX_QUEUE_SIZE, sizeof(uint8_t));
    spp_tx_queue = xQueueCreate(TX_QUEUE_SIZE, sizeof(spp_packet_t *));
    xTaskCreatePinnedToCore(spp_tx_task, "spp_tx", 4096, NULL, 2, &spp_task_handle, 0);
    spp_tx_done = xSemaphoreCreateBinary();
    xSemaphoreTake(spp_tx_done, 0);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(NULL), "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(NULL), "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(NULL), "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(NULL), "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(NULL), "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(NULL), "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_init(ESP_SPP_MODE_CB)) != ESP_OK)
    {
        ESP_LOGE(pcTaskGetName(NULL), "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);
}

size_t spp_write(const uint8_t *buffer, size_t size)
{
    if (buffer == NULL || size == (size_t)0 || spp_tx_queue == NULL)
    {
        return (size_t)0;
    }

    spp_packet_t *packet = (spp_packet_t *)pvPortMalloc(sizeof(spp_packet_t) + size);
    if (!packet)
    {
        return (size_t)0;
    }
    packet->len = size;
    (void)memcpy(packet->data, buffer, size);
    if (xQueueSend(spp_tx_queue, &packet, SPP_TX_QUEUE_TIMEOUT) != pdPASS)
    {
        vPortFree(packet);
        return (size_t)0;
    }

    return size;
}

int spp_read(void)
{
    uint8_t c;

    if (spp_rx_queue && xQueueReceive(spp_rx_queue, &c, 0) == pdTRUE)
    {
        return c;
    }

    return -1;
}

size_t spp_bytes_received_size(void)
{
	if (!spp_rx_queue)
	{
		return (size_t)0;
	}
	
	return (size_t)uxQueueMessagesWaiting(spp_rx_queue);
}
