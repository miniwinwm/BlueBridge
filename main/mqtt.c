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

#include <string.h>
#include <stdbool.h>
#include "mqtt.h"
#include "modem.h"
#include "modem_interface.h"

/**************
*** DEFINES ***
**************/

/************
*** TYPES ***
************/

/********************************
*** LOCAL FUNCTION PROTOTYPES ***
********************************/

static uint8_t EncodeRemainingLength(size_t remainingLength, uint8_t buffer[4]);	
static size_t DecodeRemainingLength(uint8_t buffer[4]);

/**********************
*** LOCAL VARIABLES ***
**********************/

static PublishCallback_t publishCallback;						///< Copy of suppled publish callback function pointer
static PingResponseCallback_t pingCallback;						///< Copy of suppled ping callback function pointer
static SubscribeResponseCallback_t subscribeCallback;			///< Copy of suppled subscribe acknowledge callback function pointer
static UnsubscribeResponseCallback_t unsubscribeCallback;		///< Copy of suppled unsubscribe acknowledge callback function pointer

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
 * Encode a remaining length value into a variable length integer array
 *
 * @param remainingLength The value to encode
 * @param buffer Buffer to put result
 * @return The number of bytes encoded into buffer
 */
static uint8_t EncodeRemainingLength(size_t remainingLength, uint8_t buffer[4])
{
	uint8_t i = 0U;
	uint8_t encodedByte;

	do
	{
		encodedByte = (uint8_t)(remainingLength % (size_t)0x80);
		remainingLength = remainingLength / (size_t)0x80;

		// if there are more data to encode, set the top bit of this byte
		if (remainingLength > (size_t)0)
		{
			encodedByte = encodedByte | 0x80U;
		}
		buffer[i] = encodedByte;
		i++;
	}
	while (remainingLength > (size_t)0);

	return i;
}

/**
 * Decode a variable length integer
 * 
 * @param buffer Buffer containing the variable length integer
 * @return The decoded integer representation of buffer
 */
static size_t DecodeRemainingLength(uint8_t buffer[4])
{
    size_t multiplier = (size_t)1;
    size_t value = (size_t)0;
    uint8_t i = 0U;
    uint8_t encodedByte;

    do
    {
         encodedByte = buffer[i];
         i++;
         value += (size_t)(encodedByte & 0x7fU) * multiplier;
         multiplier *= (size_t)0x80;
    }
    while ((encodedByte & 0x80U) != 0U);

	return value;
}

/***********************
*** GLOBAL FUNCTIONS ***
***********************/

void MqttSetPublishCallback(PublishCallback_t callback)
{
	publishCallback = callback;
}

void MqttSetPingResponseCallback(PingResponseCallback_t callback)
{
	pingCallback = callback;
}

void MqttSetSubscribeResponseCallback(SubscribeResponseCallback_t callback)
{
	subscribeCallback = callback;
}

void MqttSetUnsubscribeResponseCallback(UnsubscribeResponseCallback_t callback)
{
	unsubscribeCallback = callback;
}

MqttStatus_t MqttConnect(const char *clientId, const char *username, const char *password, uint16_t keepAlive, uint32_t timeoutMs)
{
	uint8_t remainingLengthBuffer[4];
	size_t remainingLength;
	size_t packetLength;
	uint8_t remainingLengthLength;
	uint8_t *packet;
	uint32_t p = 0UL;
	uint32_t startTime = modem_interface_get_time_ms();
	MqttStatus_t mqttStatus;

	// check parameters
	if (clientId == NULL)
	{
		return MQTT_BAD_PARAMETER;
	}

	// calculate remaining length
	remainingLength = (size_t)12 + strlen(clientId);
	if (username)
	{
		remainingLength += (size_t)2;
		remainingLength += strlen(username);
	}
	if (password)
	{
		remainingLength += (size_t)2;
		remainingLength += strlen(password);
	}

	// encode remaining length
	remainingLengthLength = EncodeRemainingLength(remainingLength, remainingLengthBuffer);

	// calculate length of packet and allocate memory
	packetLength = (size_t)1 + (size_t)remainingLengthLength + remainingLength;
	packet = modem_interface_malloc(packetLength);
	if (packet == NULL)
	{
		return MQTT_NO_MEMORY;
	}

	// packet type
	packet[p] = MQTT_CONNECT_REQ_PACKET_ID;
	p++;

	// remaining length
	(void)memcpy(&packet[p], remainingLengthBuffer, (size_t)remainingLengthLength);
	p += (uint32_t)remainingLengthLength;

	(void)memcpy(&packet[p], "\x00\x04MQTT\x04", 7);
	p += 7UL;

	// flags
	packet[p] = 0x02U;
	if (username)
	{
		packet[p] |= 0x80U;
	}
	if (password)
	{
		packet[p] |= 0x40U;
	}
	p++;

	// keepalive time
	packet[p] = (uint8_t)(keepAlive >> 8);
	p++;
	packet[p] = (uint8_t)keepAlive;
	p++;

	// client id length
	packet[p] = (uint8_t)(strlen(clientId) / (size_t)0xff);
	p++;
	packet[p] = (uint8_t)(strlen(clientId) & (size_t)0xff);
	p++;

	// client id
	(void)memcpy(&packet[p], clientId, strlen(clientId));
	p += (uint32_t)strlen(clientId);

	// username if supplied
	if (username)
	{
		packet[p] = (uint8_t)(strlen(username) / (size_t)0xff);
		p++;
		packet[p] = (uint8_t)(strlen(username) & (size_t)0xff);
		p++;

		(void)memcpy(&packet[p], username, strlen(username));
		p += (uint32_t)strlen(username);
	}

	// password if supplied
	if (password)
	{
		packet[p] = (uint8_t)(strlen(password) / (size_t)0xff);
		p++;
		packet[p] = (uint8_t)(strlen(password) & (size_t)0xff);
		p++;

		(void)memcpy(&packet[p], password, strlen(password));
		p += (uint32_t)strlen(password);
	}

	// send packet
	if (ModemTcpWrite(packet, packetLength, timeoutMs) != MODEM_SEND_OK)
	{
		modem_interface_free(packet);
		return MQTT_TCP_ERROR;
	}

	// deallocate
	modem_interface_free(packet);

	timeoutMs -= (modem_interface_get_time_ms() - startTime);

	// wait for response
	while (true)
	{
		mqttStatus = MqttHandleResponse(timeoutMs);
		if (mqttStatus != MQTT_NO_RESPONSE)
		{
			break;
		}

		modem_interface_task_delay(250UL);

		if (modem_interface_get_time_ms() > startTime + timeoutMs)
		{
			mqttStatus = MQTT_TIMEOUT;
			break;
		}
	}

	return mqttStatus;
}

MqttStatus_t MqttPing(uint32_t timeoutMs)
{
	// create packet
	const uint8_t packet[2] = {MQTT_PING_REQ_PACKET_ID, 0x00U};

	// send packet
	if (ModemTcpWrite(packet, (size_t)2, timeoutMs) != MODEM_SEND_OK)
	{
		return MQTT_TCP_ERROR;
	}

	return MQTT_OK;
}

MqttStatus_t MqttSubscribe(const char *topic, uint16_t packetIdentifier, uint32_t timeoutMs)
{
	uint8_t *packet;
	size_t remainingLength;
	size_t packetLength;
	uint8_t remainingLengthBuffer[4];
	uint8_t remainingLengthLength;
	uint32_t p = 0UL;

	// check parameters
	if (topic == NULL || strlen(topic) == (size_t)0 || strlen(topic) > (size_t)250)
	{
		return MQTT_BAD_PARAMETER;
	}

	// encode remaining length
	remainingLength = (size_t)5 + strlen(topic);
	remainingLengthLength = EncodeRemainingLength(remainingLength, remainingLengthBuffer);

	// calculate packet length and allocate memory
	packetLength = (size_t)1 + (uint32_t)remainingLengthLength + remainingLength;
	packet = modem_interface_malloc(packetLength);
	if (packet == NULL)
	{
		return MQTT_NO_MEMORY;
	}

	// packet type
	packet[p] = MQTT_SUBSCRIBE_REQ_PACKET_ID | 0x02U;
	p++;

	// remaining length
	(void)memcpy(&packet[p], remainingLengthBuffer, (size_t)remainingLengthLength);
	p += (uint32_t)remainingLengthLength;

	// packet identifier
	packet[p] = (uint8_t)(packetIdentifier >> 8);
	p++;
	packet[p] = (uint8_t)packetIdentifier;
	p++;

	// topic length
	packet[p] = 0x00U;
	p++;
	packet[p] = (uint8_t)strlen(topic);
	p++;

	// topic
	(void)memcpy(&packet[p], topic, strlen(topic));
	p += (uint32_t)strlen(topic);

	// qos
	packet[p] = 0x00U;

	// send packet
	if (ModemTcpWrite(packet, packetLength, timeoutMs) != MODEM_SEND_OK)
	{
		modem_interface_free(packet);
		return MQTT_TCP_ERROR;
	}

	// deallocate
	modem_interface_free(packet);

	return MQTT_OK;
}

MqttStatus_t MqttUnsubscribe(const char *topic, uint16_t packetIdentifier, uint32_t timeoutMs)
{
	uint8_t *packet;
	size_t remainingLength;
	size_t packetLength;
	uint8_t remainingLengthBuffer[4];
	uint8_t remainingLengthLength;
	uint32_t p = 0UL;

	// check parameters
	if (topic == NULL || strlen(topic) == (size_t)0 || strlen(topic) > (size_t)250)
	{
		return MQTT_BAD_PARAMETER;
	}

	// encode remaining length
	remainingLength = (size_t)4 + strlen(topic);
	remainingLengthLength = EncodeRemainingLength(remainingLength, remainingLengthBuffer);

	// calculate packet length and allocate memory
	packetLength = (size_t)1 + (uint32_t)remainingLengthLength + remainingLength;
	packet = modem_interface_malloc(packetLength);
	if (packet == NULL)
	{
		return MQTT_NO_MEMORY;
	}

	// packet type
	packet[p] = MQTT_UNSUBSCRIBE_REQ_PACKET_ID | 0x02U;
	p++;

	// remaining length
	(void)memcpy(&packet[p], remainingLengthBuffer, (size_t)remainingLengthLength);
	p += (uint32_t)remainingLengthLength;

	// packet identifier
	packet[p] = (uint8_t)(packetIdentifier >> 8);
	p++;
	packet[p] = (uint8_t)packetIdentifier;
	p++;

	// topic length
	packet[p] = 0x00U;
	p++;
	packet[p] = (uint8_t)strlen(topic);
	p++;

	// topic
	(void)memcpy(&packet[p], topic, strlen(topic));
	p += (uint32_t)strlen(topic);

	// send packet
	if (ModemTcpWrite(packet, packetLength, timeoutMs) != MODEM_SEND_OK)
	{
		modem_interface_free(packet);
		return MQTT_TCP_ERROR;
	}

	// deallocate
	modem_interface_free(packet);

	return MQTT_OK;
}

MqttStatus_t MqttPublish(const char *topic, const uint8_t *payload, uint32_t payloadLength, bool retain, uint32_t timeoutMs)
{
	size_t remainingLength;
	size_t packetLength;
	uint8_t remainingLengthBuffer[4] = {0};
	uint8_t remainingLengthLength;
	uint8_t *packet;
	uint32_t p = 0UL;

	// check parameters
	if (topic == NULL || payload == NULL || strlen(topic) == (size_t)0 || strlen(topic) > (size_t)250)
	{
		return MQTT_BAD_PARAMETER;
	}

	// encode remaining length
	remainingLength = (size_t)2 + strlen(topic) + payloadLength;
	remainingLengthLength = EncodeRemainingLength(remainingLength, remainingLengthBuffer);

	// calculate packet length and allocate memory
	packetLength = (size_t)1 + (uint32_t)remainingLengthLength + remainingLength;
	packet = modem_interface_malloc(packetLength);
	if (packet == NULL)
	{
		return MQTT_NO_MEMORY;
	}

	// packet type
	packet[p] = MQTT_PUBLISH_PACKET_ID;
	if (retain)
	{
		packet[p] |= 0x01U;
	}
	p++;

	// remaining length
	(void)memcpy(&packet[p], remainingLengthBuffer, (size_t)remainingLengthLength);
	p += (uint32_t)remainingLengthLength;

	// topic length
	packet[p] = 0x00U;
	p++;
	packet[p] = (uint8_t)strlen(topic);
	p++;

	// topic
	(void)memcpy(&packet[p], topic, strlen(topic));
	p += strlen(topic);

	// payload
	(void)memcpy(&packet[p], payload, payloadLength);
	p += payloadLength;

	// send packet
	if (ModemTcpWrite(packet, packetLength, timeoutMs) != MODEM_SEND_OK)
	{
		modem_interface_free(packet);
		return MQTT_TCP_ERROR;
	}

	// deallocate
	modem_interface_free(packet);

	return MQTT_OK;
}

MqttStatus_t MqttDisconnect(uint32_t timeoutMs)
{
	const uint8_t packet[4] = {MQQT_DISCONNECT_PACKET_ID, 0x00U, 0x00U, 0x00U};

	ModemStatus_t modemStatus = ModemTcpWrite(packet, sizeof(packet), timeoutMs);
	if (modemStatus != MODEM_SEND_OK)
	{
		return MQTT_TCP_ERROR;
	}

	return MQTT_OK;
}

MqttStatus_t MqttHandleResponse(uint32_t timeoutMs)
{
	uint8_t packetType;
	size_t lengthRead;
	uint8_t remainingLengthBuffer[4];
	uint32_t startTime = modem_interface_get_time_ms();
	size_t bytesWaiting;
	uint8_t i = 0U;
	size_t bytesRead;
	size_t remainingLength;
	uint8_t *remainingData = NULL;
	MqttStatus_t mqttStatus;

	if (ModemGetTcpReadDataWaitingLength(&bytesWaiting, timeoutMs) == MODEM_OK)
	{
		timeoutMs -= (modem_interface_get_time_ms() - startTime);

		if (bytesWaiting > (size_t)0)
		{
			// read response type
			if (ModemTcpRead((size_t)1, &lengthRead, &packetType, timeoutMs) != MODEM_OK)
			{
				return MQTT_TCP_ERROR;
			}
			timeoutMs -= (modem_interface_get_time_ms() - startTime);

			// read remaining length
			while (true)
			{
				if (ModemGetTcpReadDataWaitingLength(&bytesWaiting, timeoutMs) != MODEM_OK)
				{
					return MQTT_TCP_ERROR;
				}
				timeoutMs -= (modem_interface_get_time_ms() - startTime);

				if (bytesWaiting > (size_t)0)
				{
					if (ModemTcpRead((size_t)1, &bytesRead, &remainingLengthBuffer[i], timeoutMs) != MODEM_OK)
					{
						return MQTT_TCP_ERROR;
					}
					timeoutMs -= (modem_interface_get_time_ms() - startTime);

					if ((remainingLengthBuffer[i] & 0x80U) == 0x00U)
					{
						break;
					}

					i++;
					if (i == 5U)
					{
						return MQTT_UNEXPECTED_RESPONSE;
					}
				}
				else
				{
					modem_interface_task_delay(250UL);

					if (modem_interface_get_time_ms() > startTime + timeoutMs)
					{
						return MQTT_TIMEOUT;
					}
				}
			}

			// decode remaining length
			remainingLength = DecodeRemainingLength(remainingLengthBuffer);
			if (remainingLength > (size_t)0)
			{
				remainingData = modem_interface_malloc(remainingLength);
				if (remainingData == NULL)
				{
					return MQTT_NO_MEMORY;
				}

				// read remaining data
				while (true)
				{
					if (ModemGetTcpReadDataWaitingLength(&bytesWaiting, timeoutMs) != MODEM_OK)
					{
						modem_interface_free(remainingData);
						return MQTT_TCP_ERROR;
					}
					timeoutMs -= (modem_interface_get_time_ms() - startTime);

					if (bytesWaiting >= remainingLength)
					{
						if (ModemTcpRead(remainingLength, &bytesRead, remainingData, timeoutMs) != MODEM_OK)
						{
							modem_interface_free(remainingData);
							return MQTT_TCP_ERROR;
						}
						timeoutMs -= (modem_interface_get_time_ms() - startTime);
						break;
					}
					else
					{
						modem_interface_task_delay(250UL);

						if (modem_interface_get_time_ms() > startTime + timeoutMs)
						{
							modem_interface_free(remainingData);
							return MQTT_TIMEOUT;
						}
					}
				}
			}	

			if ((packetType & MQTT_PACKET_ID_MASK) == MQTT_PUBLISH_PACKET_ID && publishCallback)
			{
				if (remainingLength < (size_t)6)
				{
					mqttStatus = MQTT_UNEXPECTED_RESPONSE;
				}
				else
				{
					// create a NULL terminated string for topic, this overwrites payload length first byte but it's not being used
					remainingData[(uint32_t)remainingData[1] + 2UL] = '\0';
					
					// publish response
					publishCallback((char *)&remainingData[2], 					// topic starts at byte 3
							remainingData + (uint32_t)remainingData[1] + 2UL, 	// payload starts at byte (topic length + 2)
							remainingLength - (size_t)remainingData[1] - (size_t)2UL);
					mqttStatus = MQTT_PUBLISH;
				}
			}
			else if ((packetType & MQTT_PACKET_ID_MASK) == MQTT_PING_RESP_PACKET_ID && pingCallback)
			{
				// ping ack
				pingCallback();
				mqttStatus = MQTT_PING_ACK;
			}
			else if ((packetType & MQTT_PACKET_ID_MASK) == MQTT_SUBSCRIBE_ACK_PACKET_ID && subscribeCallback)
			{
				// subscribe ack
				if (remainingLength != (size_t)3 || (remainingData[2] != 0x00U && remainingData[2] != 0x80U))
				{
					mqttStatus = MQTT_UNEXPECTED_RESPONSE;
				}
				else
				{
					subscribeCallback(((uint16_t)remainingData[0] << 8) + (uint16_t)remainingData[1], remainingData[2] == 0x00U);
					mqttStatus = MQTT_SUBSCRIBE_ACK;
				}
			}
			else if ((packetType & MQTT_PACKET_ID_MASK) == MQTT_UNSUBSCRIBE_ACK_PACKET_ID && unsubscribeCallback)
			{
				// unsubscribe ack
				if (remainingLength != (size_t)2)
				{
					mqttStatus = MQTT_UNEXPECTED_RESPONSE;
				}
				else
				{
					unsubscribeCallback(((uint16_t)remainingData[0] << 8) + (uint16_t)remainingData[1]);
					mqttStatus = MQTT_SUBSCRIBE_ACK;
				}
			}
			else if ((packetType & MQTT_PACKET_ID_MASK) == MQTT_CONNECT_ACK_PACKET_ID)
			{
				if (remainingLength != (size_t)2)
				{
					mqttStatus = MQTT_UNEXPECTED_RESPONSE;
				}
				if (remainingData[0] != 0x00U)
				{
					mqttStatus = MQTT_CONNECTION_REFUSED;
				}
				else
				{
					mqttStatus = MQTT_OK;
				}
			}
			else
			{
				mqttStatus = MQTT_OK;
			}

			modem_interface_free(remainingData);
			return mqttStatus;
		}
	}

	return MQTT_NO_RESPONSE;
}

const char *MqttStatusToText(MqttStatus_t mqttStatus)
{
	switch (mqttStatus)
	{
	case MQTT_OK:
		return "MQTT_OK";

	case MQTT_CONNECTION_REFUSED:
		return "MQTT_CONNECTION_REFUSED";

	case MQTT_TIMEOUT:
		return "MQTT_TIMEOUT";

	case MQTT_NO_RESPONSE:
		return "MQTT_NO_RESPONSE";

	case MQTT_UNEXPECTED_RESPONSE:
		return "MQTT_UNEXPECTED_RESPONSE";

	case MQTT_BAD_PARAMETER:
		return "MQTT_BAD_PARAMETER";

	case MQTT_NO_MEMORY:
		return "MQTT_NO_MEMORY";

	case MQTT_TCP_ERROR:
		return "MQTT_TCP_ERROR";

	case MQTT_SUBSCRIBE_FAILURE:
		return "MQTT_SUBSCRIBE_FAILURE";

	case MQTT_PING_ACK:
		return "MQTT_PING_ACK";

	case MQTT_SUBSCRIBE_ACK:
		return "MQTT_SUBSCRIBE_ACK";

	case MQTT_PUBLISH:
		return "MQTT_PUBLISH";

	default:
		return "UNKNOWN_STATUS";
	}
}
