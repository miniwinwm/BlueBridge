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

#ifndef MQTT_H
#define MQTT_H

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

#define MQTT_CONNECT_REQ_PACKET_ID		0x10U			///< MQTT protocol packet id for connect request
#define MQTT_CONNECT_ACK_PACKET_ID		0x20U			///< MQTT protocol packet id for connect acknowledge
#define MQTT_PUBLISH_PACKET_ID			0x30U			///< MQTT protocol packet id for publish
#define MQTT_SUBSCRIBE_REQ_PACKET_ID	0x80U			///< MQTT protocol packet id for subscribe request
#define MQTT_SUBSCRIBE_ACK_PACKET_ID	0x90U			///< MQTT protocol packet id for subscribe acknowledge
#define MQTT_UNSUBSCRIBE_REQ_PACKET_ID	0xa0U			///< MQTT protocol packet id for unsubscribe request
#define MQTT_UNSUBSCRIBE_ACK_PACKET_ID	0xb0U			///< MQTT protocol packet id for unsubscribe acknowledge
#define MQTT_PING_REQ_PACKET_ID			0xc0U			///< MQTT protocol packet id for ping request
#define MQTT_PING_RESP_PACKET_ID		0xd0U			///< MQTT protocol packet id for ping response
#define MQQT_DISCONNECT_PACKET_ID		0xe0			///< MQTT protocol packet id for disconnect
#define MQTT_PACKET_ID_MASK				0xf0U			///< MQTT protocol packet id mask

/**
 * Response and error codes from this API
 */
typedef enum
{
	// ok statuses
	MQTT_OK = 0,					///< No error
	MQTT_NO_RESPONSE = 1,			///< No response was received
	MQTT_PING_ACK = 2,				///< Ping acknowledgement received
	MQTT_SUBSCRIBE_ACK = 3,			///< Subscribe acknowledgement received
	MQTT_PUBLISH = 4,				///< A publish to a subscribed topic received

	// error statuses				
	MQTT_CONNECTION_REFUSED = -1,	///< Connection was refused
	MQTT_TIMEOUT = -2,				///< No response within time limit
	MQTT_UNEXPECTED_RESPONSE = -3,	///< Response received was not expected response
	MQTT_BAD_PARAMETER = -4,		///< One of the parameters to the function call unacceptable
	MQTT_NO_MEMORY = -5,			///< malloc failed, no heap memory
	MQTT_TCP_ERROR = -6,			///< A TCP error was reported from the modem
	MQTT_SUBSCRIBE_FAILURE = -7		///< Subscribe attempt failed
} MqttStatus_t;

/************
*** TYPES ***
************/

/**
 * Callback method for when a publish message has been received
 *
 * @param topic The topic string data
 * @param payload The message payload bytes
 * @param payloadLength The length of the payload in bytes
 * @note When this function exits the buffers containing topic and payload are deleted. Make local copies if required.
 */
typedef void (*PublishCallback_t)(const char *topic, const uint8_t *payload, size_t payloadLength);

/**
 * Callback method for when a ping response message has been received
 */
typedef void (*PingResponseCallback_t)(void);

/**
 * Callback method for when a subscribe response message has been received
 *
 * @param packetIdentifier The packet identifier used during the subscribe
 * @param success If the subscribe was successful
 */
typedef void (*SubscribeResponseCallback_t)(uint16_t packetIdentifier, bool success);

/**
 * Callback method for when an unsubscribe response message has been received
 *
 * @param packetIdentifier The packet identifier used during the subscribe
 */
typedef void (*UnsubscribeResponseCallback_t)(uint16_t packetIdentifier);

/*************************
*** EXTERNAL VARIABLES ***
*************************/

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Set the callback function to be called when a published message arrives
 *
 * @param callback The function that is called
 */
void MqttSetPublishCallback(PublishCallback_t callback);


/**
 * Set the callback function to be called when a ping response message arrives
 *
 * @param callback The function that is called
 */
void MqttSetPingResponseCallback(PingResponseCallback_t callback);

/**
 * Set the callback function to be called when a subscribe acknowledge message arrives
 *
 * @param callback The function that is called
 */
void MqttSetSubscribeResponseCallback(SubscribeResponseCallback_t callback);

/**
 * Set the callback function to be called when an unsubscribe acknowledge message arrives
 *
 * @param callback The function that is called
 */
void MqttSetUnsubscribeResponseCallback(UnsubscribeResponseCallback_t callback);

/**
 * Connect to a MQTT broker
 *
 * @param clientId String containing client id sent in connect message to broker
 * @param username String containing user name sent in connect message to broker, can be NULL
 * @param password String containing password sent in connect message to broker, can be NULL
 * @param keepAlive Time in seconds that the connection should be kept alive by broker
 * @param timeoutMs Timeout in milliseconds to wait for a successful connection
 * @return One of the MQTT defined responses or errors
 */
MqttStatus_t MqttConnect(const char *clientId, const char *username, const char *password, uint16_t keepAlive, uint32_t timeoutMs);


/**
 * Send a ping message to the broker 
 *
 * @param timeoutMs Timeout in milliseconds to wait for a successful sending
 * @return One of the MQTT defined responses or errors
 */
 MqttStatus_t MqttPing(uint32_t timeoutMs);

/**
 * Publish a topic/value pair to the broker 
 *
 * @param topic Null terminated tring containing the publish message topic
 * @param payload Binary array containing the payload
 * @param payloadLength Length of data in payload
 * @param retain The value of the retain flag to send in the message
 * @param timeoutMs Timeout in milliseconds to wait for a successful publishing
 * @return One of the MQTT defined responses or errors
 */
MqttStatus_t MqttPublish(const char *topic, const uint8_t *payload, size_t payloadLength, bool retain, uint32_t timeoutMs);

/**
 * Subscribe to a topic at the broker 
 *
 * @param topic Null terminated string containing the publish message topic
 * @param packetIdentifier Packet identifier that will be used in the subscribe acknowledge message
 * @param timeoutMs Timeout in milliseconds to wait for a successful subscribing
 * @return One of the MQTT defined responses or errors
 */
MqttStatus_t MqttSubscribe(const char *topic, uint16_t packetIdentifier, uint32_t timeoutMs);

/**
 * Unsubscribe from a topic at the broker 
 *
 * @param topic Null terminated string containing the publish message topic
 * @param packetIdentifier Packet identifier that will be used in the unsubscribe acknowledge message
 * @param timeoutMs Timeout in milliseconds to wait for a successful unsubscribing
 * @return One of the MQTT defined responses or errors
 */
MqttStatus_t MqttUnsubscribe(const char *topic, uint16_t packetIdentifier, uint32_t timeoutMs);

/**
 * Disconnect from the broker
 *
 * @param timeoutMs Timeout in milliseconds to wait for a successful disconnecting
 * @return One of the MQTT defined responses or errors
 */
MqttStatus_t MqttDisconnect(uint32_t timeoutMs);

/**
 * Call this function periodically to handle all responses and acknowledgements from the broker
 *
 * @param timeoutMs Timeout in milliseconds to wait for a successful unsubscribing
 * @return One of the MQTT defined responses or errors
 */
MqttStatus_t MqttHandleResponse(uint32_t timeoutMs);

/**
 * Convert an enumed value of status to text
 *
 * @param mqttStatus The enum value to convert
 * @return The text equivalent
 */
const char *MqttStatusToText(MqttStatus_t mqttStatus);

#ifdef __cplusplus
}
#endif

#endif
