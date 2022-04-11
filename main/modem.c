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

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "modem.h"
#include "modem_interface.h"

// server side of client API for each AT command type
static void ServerModemHello(uint32_t timeoutMs);
static void ServerGetSignalStrength(uint32_t timeoutMs);
static void ServerNetworkRegistrationStatus(uint32_t timeoutMs);
static void ServerGetOperatorDetails(uint32_t timeoutMs);
static void ServerSetManualDataReceive(uint32_t timeoutMs);
static void ServerSetSmsPduMode(uint32_t timeoutMs);
static void ServerSetSmsReceiveMode(uint32_t timeoutMs);
static void ServerSmsReceiveMessage(uint32_t timeoutMs);
static void ServerSmsSendMessage(uint32_t timeoutMs);
static void ServerSmsDeleteAllMessages(uint32_t timeoutMs);
static void ServerActivateDataConnection(uint32_t timeoutMs);
static void ServerConfigureDataConnection(uint32_t timeoutMs);
static void ServerDeactivateDataConnection(uint32_t timeoutMs);
static void ServerOpenTcpConnection(uint32_t timeoutMs);
static void ServerTcpWrite(uint32_t timeoutMs);
static void ServerCloseTcpConnection(uint32_t timeoutMs);
static void ServerGetOwnIpAddress(uint32_t timeoutMs);
static void ServerGetTcpReadDataWaitingLength(uint32_t timeoutMs);
static void ServerTcpRead(uint32_t timeoutMs);
static void ServerPowerDown(uint32_t timeoutMs);
static void ServerGetImei(uint32_t timeoutMs);

// urc
static void ServerHandleURC(size_t length);
static bool tcpConnectedState = false;
static bool pdpActivatedState = false;

// utility functions
static ModemStatus_t ServerSendBasicCommandResponse(char *command, uint32_t timeoutMs);
static ModemStatus_t ServerSendBasicCommandTextResponse(char *command, char *response, size_t response_length, uint32_t timeoutMs);
static ModemStatus_t ServerGetEcho(char *command, uint32_t timeoutMs);
static ModemStatus_t ServerGetStandardResponse(uint32_t timeoutMs);
static ModemStatus_t ClientSendBasicCommandResponse(AtCommand_t atCommand, uint32_t timeoutMs);
static ModemStatus_t ClientTcpWriteSection(const uint8_t *data, size_t length, uint32_t timeoutMs);
static ModemStatus_t ClientTcpReadSection(size_t lengthToRead, size_t *lengthRead, uint8_t *buffer, uint32_t timeoutMs);
static void ServerFlushReadBufferOnError(ModemStatus_t modemStatus);

// server objects
static uint8_t echoOrUrc[MODEM_MAX_AT_COMMAND_SIZE];
static AtCommandPacket_t atCommandPacket;
static AtResponsePacket_t atResponsePacket;

static SmsNotificationCallback_t mySmsNotificationCallback;

ModemStatus_t ModemSetSmsNotificationCallback(SmsNotificationCallback_t smsNotificationCallback)
{
	if (smsNotificationCallback == NULL)
	{
		return MODEM_BAD_PARAMETER;
	}
	
	mySmsNotificationCallback = smsNotificationCallback;
	
	return MODEM_OK;
}

void ModemReset(void)
{
	uint8_t byte;
	
	(void)modem_interface_serial_write_data((size_t)13, (uint8_t *)"AT+CFUN=1,1\r\n");
	modem_interface_task_delay(3000UL);

	// read bytes from buffer until buffer empty or a '\n' is received
	do
	{
		modem_interface_serial_read_data((size_t)1, &byte);
	}
	while (byte != '\n' && modem_interface_serial_received_bytes_waiting() > (size_t)0);	
}

void ModemDelete(void)
{
	modem_interface_os_deinit();
	modem_interface_serial_close();
}

ModemStatus_t ModemInit(void)
{
	uint8_t initResponse[200];
	uint8_t tries = 0U;
	ModemStatus_t status = MODEM_NO_RESPONSE;
	
	tcpConnectedState = false;
	pdpActivatedState = false;
	modem_interface_serial_init();
	ModemReset();

	while (tries < 10U)
	{
		(void)modem_interface_serial_write_data((size_t)6, (uint8_t *)"ATE1\r\n");
		modem_interface_task_delay(100UL);

		(void)modem_interface_serial_read_data((size_t)20, initResponse);
		modem_interface_log((const char *)initResponse);
		if (memcmp(initResponse, "ATE1\r\r\nOK\r\n", (size_t)11) == 0)
		{
			status = MODEM_OK;
			break;
		}

		if (memcmp("OK\r\n", initResponse, (size_t)4) == 0)
		{
			status = MODEM_OK;
			break;
		}

		tries++;
		modem_interface_task_delay(1000UL);
	}
	
	if (status == MODEM_NO_RESPONSE)
	{
		return status;
	}
	
	modem_interface_os_init();
	
	return status;
}

void DoModemTask(void)
{
	uint32_t nextUnsolicitedResponsePos;
	uint32_t urcReceiveStartTimeMs;
	
	modem_interface_log("Modem task started");

	while (true)
	{
		modem_interface_task_delay(MODEM_SERVER_LOOP_PERIOD_MS);

		// check for out of sequence data arriving as this means a URC has arrived
		if (modem_interface_serial_received_bytes_waiting() > (size_t)0 && modem_interface_acquire_mutex(0UL) == MODEM_OK)
		{
			urcReceiveStartTimeMs = modem_interface_get_time_ms();
			nextUnsolicitedResponsePos = 0UL;

			while (true)
			{
				// check for overflow
				if (nextUnsolicitedResponsePos == MODEM_MAX_URC_LENGTH)
				{
					nextUnsolicitedResponsePos = 0UL;
				}

				// check for timeoutMs
				if (modem_interface_get_time_ms() > urcReceiveStartTimeMs + MODEM_URC_TIMEOUT_MS)
				{
					modem_interface_release_mutex();
					break;
				}

				// check for more data waiting
				if (modem_interface_serial_received_bytes_waiting() > (size_t)0)
				{
					modem_interface_serial_read_data((size_t)1, &echoOrUrc[nextUnsolicitedResponsePos]);
					if (echoOrUrc[nextUnsolicitedResponsePos] == '\n')
					{
						// ignore \r\n blank lines
						if (memcmp(echoOrUrc, "\r\n", (size_t)2) != 0)
						{
							ServerHandleURC((size_t)(nextUnsolicitedResponsePos + 1UL));
						}
						modem_interface_release_mutex();
						break;
					}
					nextUnsolicitedResponsePos++;
				}
			}
		}

		if (modem_interface_queue_get(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) == MODEM_OK)
		{
			if (modem_interface_acquire_mutex(atCommandPacket.timeoutMs) != MODEM_OK)
			{
				atResponsePacket.atResponse = MODEM_TIMEOUT;
				modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
			}
			else
			{
				switch (atCommandPacket.atCommand)
				{
					case MODEM_COMMAND_HELLO:
						ServerModemHello(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_SIGNAL_STRENGTH:
						ServerGetSignalStrength(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_NETWORK_REGISTRATION:
						ServerNetworkRegistrationStatus(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_GET_OPERATOR_DETAILS:
						ServerGetOperatorDetails(atCommandPacket.timeoutMs);
						break;
						
					case MODEM_COMMAND_SET_MANUAL_DATA_READ:
						ServerSetManualDataReceive(atCommandPacket.timeoutMs);
						break;
						
					case MODEM_COMMAND_SET_SMS_PDU_MODE:
						ServerSetSmsPduMode(atCommandPacket.timeoutMs);
						break;						

					case MODEM_COMMAND_SET_SMS_RECEIVE_MODE:
						ServerSetSmsReceiveMode(atCommandPacket.timeoutMs);
						break;	
						
					case MODEM_COMMAND_SMS_RECEIVE_MESSAGE:
						ServerSmsReceiveMessage(atCommandPacket.timeoutMs);
						break;				

					case MODEM_COMMAND_SMS_SEND_MESSAGE:
						ServerSmsSendMessage(atCommandPacket.timeoutMs);					
						break;
						
					case MODEM_COMMAND_SMS_DELETE_ALL_MESSAGEs:
						ServerSmsDeleteAllMessages(atCommandPacket.timeoutMs);					
						break;						

					case MODEM_COMMAND_ACTIVATE_DATA_CONNECTION:
						ServerActivateDataConnection(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_CONFIGURE_DATA_CONNECTION:
						ServerConfigureDataConnection(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_DEACTIVATE_DATA_CONNECTION:
						ServerDeactivateDataConnection(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_OPEN_TCP_CONNECTION:
						ServerOpenTcpConnection(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_CLOSE_TCP_CONNECTION:
						ServerCloseTcpConnection(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_GET_OWN_IP_ADDRESS:
						ServerGetOwnIpAddress(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_TCP_WRITE:
						ServerTcpWrite(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_GET_TCP_READ_DATA_WAITING_LENGTH:
						ServerGetTcpReadDataWaitingLength(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_TCP_READ:
						ServerTcpRead(atCommandPacket.timeoutMs);
						break;

					case MODEM_COMMAND_POWER_DOWN:
						ServerPowerDown(atCommandPacket.timeoutMs);
						break;
						
					case MODEM_COMMAND_GET_IMEI:
						ServerGetImei(atCommandPacket.timeoutMs);
						break;
					}
				modem_interface_release_mutex();
			}
		}
	}
}

static void ServerHandleURC(size_t length)
{
	if (memcmp(echoOrUrc, "CONNECT OK\r\n", 12) == 0)
	{
		tcpConnectedState = true;
	}
	else if (memcmp(echoOrUrc, "CLOSED\r\n", 8) == 0)
	{		
		tcpConnectedState = false;
	}
	else if (memcmp(echoOrUrc, "+PDP: DEACT\r\n", 8) == 0)
	{
		pdpActivatedState = false;
	}	
	else if (memcmp(echoOrUrc, "+CMTI: \"", 8) == 0)
	{
		uint32_t smsId;
		sscanf((char *)(echoOrUrc + 12), "%u", &smsId);
		if (mySmsNotificationCallback != NULL)
		{
			mySmsNotificationCallback(smsId);
		}
	}

	// add more URC handling here if needed
}

bool ModemGetTcpConnectedState(void)
{
	return tcpConnectedState;
}

bool ModemGetPdpActivatedState(void)
{
	return pdpActivatedState;
}

static ModemStatus_t ServerSendBasicCommandResponse(char *command, uint32_t timeoutMs)
{
	size_t length = strlen(command);
	ModemStatus_t modemStatus;
	uint32_t startTime = modem_interface_get_time_ms();

	modem_interface_serial_write_data(length, (uint8_t *)command);
	modem_interface_serial_write_data((size_t)1, (uint8_t *)"\r");

	modemStatus = ServerGetEcho(command, timeoutMs);
	if (modemStatus != MODEM_OK)
	{
		return modemStatus;
	}
	timeoutMs -= (modem_interface_get_time_ms() - startTime);

	return ServerGetStandardResponse(timeoutMs);
}

static ModemStatus_t ServerSendBasicCommandTextResponse(char *command, char *response, size_t response_length, uint32_t timeoutMs)
{
	size_t length = strlen(command);
	size_t i = (size_t)0;
	uint32_t startTime = modem_interface_get_time_ms();
	ModemStatus_t modemStatus;

	modem_interface_serial_write_data(length, (uint8_t *)command);
	modem_interface_serial_write_data((size_t)1, (uint8_t *)"\r");

	modemStatus = ServerGetEcho(command, timeoutMs);
	if (modemStatus != MODEM_OK)
	{
		return modemStatus;
	}
	timeoutMs -= (modem_interface_get_time_ms() - startTime);

	while (true)
	{
		if (modem_interface_serial_received_bytes_waiting() > (size_t)0)
		{
			modem_interface_serial_read_data((size_t)1, (uint8_t *)&response[i]);
			if (response[i] == '\n')
			{
				response[i] = '\0';
				break;
			}
			i++;

			if (i > response_length)
			{
				return MODEM_OVERFLOW;
			}
		}
		else
		{
			if (modem_interface_get_time_ms() > startTime + timeoutMs)
			{
				return MODEM_TIMEOUT;
			}
		}
	}
	timeoutMs -= (modem_interface_get_time_ms() - startTime);

	return ServerGetStandardResponse(timeoutMs);
}

static void ServerFlushReadBufferOnError(ModemStatus_t modemStatus)
{
	uint8_t byte;

	if (modemStatus >= MODEM_OK)
	{
		// no error
		return;
	}

	// read bytes from buffer until buffer empty or a '\n' is received
	do
	{
		modem_interface_serial_read_data((size_t)1, &byte);
	}
	while (byte != '\n' && modem_interface_serial_received_bytes_waiting() > (size_t)0);
}

static ModemStatus_t ServerGetEcho(char *command, uint32_t timeoutMs)
{
	size_t length = strlen(command);
	size_t bytesRead = (size_t)0;
	uint32_t startTime = modem_interface_get_time_ms();
	uint8_t byte;

	while (true)
	{
		if (modem_interface_serial_received_bytes_waiting() > (size_t)0)
		{
			modem_interface_serial_read_data((size_t)1, &echoOrUrc[bytesRead]);
			if (echoOrUrc[bytesRead] == '\n')
			{
				ServerHandleURC(bytesRead + (size_t)1);
				bytesRead = (size_t)0;
				continue;
			}

			if (bytesRead == length)
			{
				if (memcmp(command, echoOrUrc, length) == 0)
				{
					// echo received successfully
					break;
				}
			}

			bytesRead++;

			if (bytesRead == (size_t)MODEM_MAX_AT_COMMAND_SIZE)
			{
				return MODEM_UNEXPECTED_RESPONSE;
			}
		}
		else
		{
			if (modem_interface_get_time_ms() > startTime + timeoutMs)
			{
				return MODEM_TIMEOUT;
			}
		}
	}

	// clean up trailing \r, \n
	while (true)
	{
		if (modem_interface_serial_received_bytes_waiting() > (size_t)0)
		{
			modem_interface_serial_read_data((size_t)1, &byte);
			if (byte != '\r' && byte != '\n')
			{
				return MODEM_UNEXPECTED_RESPONSE;
			}

			if (byte == '\n')
			{
				break;
			}
		}
		else
		{
			if (modem_interface_get_time_ms() > startTime + timeoutMs)
			{
				return MODEM_TIMEOUT;
			}
		}
	}

	return MODEM_OK;
}

static ModemStatus_t ServerGetStandardResponse(uint32_t timeoutMs)
{
	size_t i = (size_t)0;
	uint32_t startTime = modem_interface_get_time_ms();
	uint8_t response[20];

	while (true)
	{
		if (modem_interface_serial_received_bytes_waiting() > (size_t)0)
		{
			modem_interface_serial_read_data((size_t)1, &response[i]);
			if (response[i] == '\n')
			{
				if (memcmp(response, "\r\n", (size_t)2) == 0)
				{
					i = 0;
					continue;
				}
				else if (memcmp(response, "OK\r\n", (size_t)4) == 0)
				{
					return MODEM_OK;
				}
				else if (memcmp(response, "SHUT OK\r\n", (size_t)9) == 0)
				{
					return MODEM_SHUT_OK;
				}
				else if (memcmp(response, "CLOSE OK\r\n", (size_t)10) == 0)
				{
					return MODEM_CLOSE_OK;
				}
				else if (memcmp(response, "SEND OK\r\n", (size_t)9) == 0)
				{
					return MODEM_SEND_OK;
				}
				else if (memcmp(response, "ERROR\r\n", (size_t)7) == 0)
				{
					return MODEM_ERROR;
				}
				else if (memcmp(response, "CLOSED\r\n", (size_t)8) == 0)
				{
					return MODEM_CLOSED;
				}
				else if (memcmp(response, "NORMAL POWER DOWN\r\n", (size_t)8) == 0)
				{
					return MODEM_POWERED_DOWN;
				}
				else
				{
					return MODEM_UNEXPECTED_RESPONSE;
				}
			}
			i++;

			if (i == sizeof(response))
			{
				return MODEM_UNEXPECTED_RESPONSE;
			}
		}
		else
		{
			if (modem_interface_get_time_ms() > startTime + timeoutMs)
			{
				return MODEM_TIMEOUT;
			}
		}
	}
}

static ModemStatus_t ClientSendBasicCommandResponse(AtCommand_t atCommand, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;

	atCommandPacket.atCommand = atCommand;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	return atResponsePacket.atResponse;
}

// modem hello client and server functions

ModemStatus_t ModemHello(uint32_t timeoutMs)
{
	return ClientSendBasicCommandResponse(MODEM_COMMAND_HELLO, timeoutMs);
}

static void ServerModemHello(uint32_t timeoutMs)
{
	atResponsePacket.atResponse = ServerSendBasicCommandResponse("AT", timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// get signal strength client and server functions

typedef struct
{
	uint8_t signalStrength;
} GetSignalStrengthResponseData_t;

ModemStatus_t ModemGetSignalStrength(uint8_t *strength, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	GetSignalStrengthResponseData_t signalStrengthResponseData;

	if (!strength)
	{
		return MODEM_BAD_PARAMETER;
	}

	atCommandPacket.atCommand = MODEM_COMMAND_SIGNAL_STRENGTH;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	memcpy(&signalStrengthResponseData, atResponsePacket.data, sizeof(signalStrengthResponseData));
	*strength = signalStrengthResponseData.signalStrength;

	return atResponsePacket.atResponse;
}

static void ServerGetSignalStrength(uint32_t timeoutMs)
{
	GetSignalStrengthResponseData_t signalStrengthResponseData;
	char responseText[20];
	char *dummy;

	atResponsePacket.atResponse = ServerSendBasicCommandTextResponse("AT+CSQ", responseText, sizeof(responseText), timeoutMs);
	if (atResponsePacket.atResponse == MODEM_OK)
	{
		if (memcmp(responseText, "+CSQ: ", (size_t)6) != 0)
		{
			atResponsePacket.atResponse = MODEM_UNEXPECTED_RESPONSE;
		}
		else
		{
			signalStrengthResponseData.signalStrength = (uint8_t)strtol(responseText + 6, &dummy, 10);
			memcpy(atResponsePacket.data, &signalStrengthResponseData, sizeof(signalStrengthResponseData));
		}
	}
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// get network registration status client and server functions

typedef struct
{
	bool registrationStatus;
} GetRegistrationStatusResponseData_t;

ModemStatus_t ModemGetNetworkRegistrationStatus(bool *registered, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	GetRegistrationStatusResponseData_t getRegistrationStatusResponseData;

	if (!registered)
	{
		return MODEM_BAD_PARAMETER;
	}

	atCommandPacket.atCommand = MODEM_COMMAND_NETWORK_REGISTRATION;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	memcpy(&getRegistrationStatusResponseData, atResponsePacket.data, sizeof(getRegistrationStatusResponseData));
	*registered = getRegistrationStatusResponseData.registrationStatus;

	return atResponsePacket.atResponse;
}

static void ServerNetworkRegistrationStatus(uint32_t timeoutMs)
{
	GetRegistrationStatusResponseData_t registrationStatusResponseData;
	uint8_t registrationStatusInt;
	char responseText[20];
	char *dummy;

	atResponsePacket.atResponse = ServerSendBasicCommandTextResponse("AT+CREG?", responseText, sizeof(responseText), timeoutMs);
	if (atResponsePacket.atResponse == MODEM_OK)
	{
		if (memcmp(responseText, "+CREG: 0,", (size_t)9) != 0)
		{
			atResponsePacket.atResponse = MODEM_UNEXPECTED_RESPONSE;
		}
		else
		{		
			registrationStatusInt = (uint8_t)strtol(responseText + 9, &dummy, 10);
			if (registrationStatusInt == 1U || registrationStatusInt == 5U)
			{
				registrationStatusResponseData.registrationStatus = true;
			}
			else
			{
				registrationStatusResponseData.registrationStatus = false;
			}
			memcpy(atResponsePacket.data, &registrationStatusResponseData, sizeof(registrationStatusResponseData));
		}
	}

	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// set manual data read client and server functions

ModemStatus_t ModemSetManualDataRead(uint32_t timeoutMs)
{
	return ClientSendBasicCommandResponse(MODEM_COMMAND_SET_MANUAL_DATA_READ, timeoutMs);
}

static void ServerSetManualDataReceive(uint32_t timeoutMs)
{
	atResponsePacket.atResponse = ServerSendBasicCommandResponse("AT+CIPRXGET=1", timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// set SMS text mode client and server functions

ModemStatus_t ModemSetSmsPduMode(uint32_t timeoutMs)
{
	return ClientSendBasicCommandResponse(MODEM_COMMAND_SET_SMS_PDU_MODE, timeoutMs);
}

static void ServerSetSmsPduMode(uint32_t timeoutMs)
{
	atResponsePacket.atResponse = ServerSendBasicCommandResponse("AT+CMGF=0", timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// set SMS receive mode client and server functions

ModemStatus_t ModemSetSmsReceiveMode(uint32_t timeoutMs)
{
	return ClientSendBasicCommandResponse(MODEM_COMMAND_SET_SMS_RECEIVE_MODE, timeoutMs);
}

static void ServerSetSmsReceiveMode(uint32_t timeoutMs)
{
	atResponsePacket.atResponse = ServerSendBasicCommandResponse("AT+CNMI=1,1,0,0,0", timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// power down modem

ModemStatus_t ModemPowerDown(uint32_t timeoutMs)
{
	return ClientSendBasicCommandResponse(MODEM_COMMAND_POWER_DOWN, timeoutMs);
}

static void ServerPowerDown(uint32_t timeoutMs)
{
	atResponsePacket.atResponse = ServerSendBasicCommandResponse("AT+CPOWD=1", timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// activate data connection client and server functions

ModemStatus_t ModemActivateDataConnection(uint32_t timeoutMs)
{
	ModemStatus_t modemStatus = ClientSendBasicCommandResponse(MODEM_COMMAND_ACTIVATE_DATA_CONNECTION, timeoutMs);
	
	if (modemStatus == MODEM_OK)
	{
		pdpActivatedState = true;
	}
	
	return modemStatus;
}

static void ServerActivateDataConnection(uint32_t timeoutMs)
{
	atResponsePacket.atResponse = ServerSendBasicCommandResponse("AT+CIICR", timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// configure data connection

typedef struct
{
	char apn[MODEM_MAX_APN_LENGTH + 1];
	char username[MODEM_MAX_USERNAME_LENGTH + 1];
	char password[MODEM_MAX_PASSWORD_LENGTH + 1];
} ConfigureDataConnectionCommandData_t;

ModemStatus_t ModemConfigureDataConnection(const char *apn, const char *username, const char *password, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	ConfigureDataConnectionCommandData_t configureDataConnectionCommandData;

	if (apn == NULL ||
			username == NULL ||
			password == NULL ||
			strlen(apn) > (size_t)MODEM_MAX_APN_LENGTH ||
			strlen(username) > (size_t)MODEM_MAX_USERNAME_LENGTH ||
			strlen(password) > (size_t)MODEM_MAX_PASSWORD_LENGTH)
	{
		return MODEM_BAD_PARAMETER;
	}

	strcpy(configureDataConnectionCommandData.apn, apn);
	strcpy(configureDataConnectionCommandData.username, username);
	strcpy(configureDataConnectionCommandData.password, password);

	memcpy(atCommandPacket.data, &configureDataConnectionCommandData, sizeof(configureDataConnectionCommandData));
	atCommandPacket.atCommand = MODEM_COMMAND_CONFIGURE_DATA_CONNECTION;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	return atResponsePacket.atResponse;
}

static void ServerConfigureDataConnection(uint32_t timeoutMs)
{
	ConfigureDataConnectionCommandData_t configureDataConnectionCommandData;
	char atCommandBuf[MODEM_MAX_AT_COMMAND_SIZE];

	memcpy(&configureDataConnectionCommandData, atCommandPacket.data, sizeof(configureDataConnectionCommandData));
	strcpy(atCommandBuf, "AT+CSTT=\"");
	strcat(atCommandBuf, configureDataConnectionCommandData.apn);
	strcat(atCommandBuf, "\",\"");
	strcat(atCommandBuf, configureDataConnectionCommandData.username);
	strcat(atCommandBuf, "\",\"");
	strcat(atCommandBuf, configureDataConnectionCommandData.password);
	strcat(atCommandBuf, "\"\r");

	atResponsePacket.atResponse = ServerSendBasicCommandResponse(atCommandBuf, timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// deactivate data connection client and server functions

ModemStatus_t ModemDeactivateDataConnection(uint32_t timeoutMs)
{
	pdpActivatedState = false;

	return ClientSendBasicCommandResponse(MODEM_COMMAND_DEACTIVATE_DATA_CONNECTION, timeoutMs);
}

static void ServerDeactivateDataConnection(uint32_t timeoutMs)
{
	atResponsePacket.atResponse = ServerSendBasicCommandResponse("AT+CIPSHUT", timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// open TCP connection client and server functions

typedef struct
{
	char url[MODEM_MAX_URL_ADDRESS_SIZE + 1];
	uint16_t port;
} OpenTcpConnectionCommandData_t;

ModemStatus_t ModemOpenTcpConnection(const char *url, uint16_t port, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	OpenTcpConnectionCommandData_t openTcpConnectionCommandData;
	uint32_t startTime = modem_interface_get_time_ms();

	if (url == NULL || strlen(url) > (size_t)MODEM_MAX_URL_ADDRESS_SIZE)
	{
		return MODEM_BAD_PARAMETER;
	}

	if (tcpConnectedState)
	{
		return MODEM_TCP_ALREADY_CONNECTED;
	}

	strcpy(openTcpConnectionCommandData.url, url);
	openTcpConnectionCommandData.port = port;

	memcpy(atCommandPacket.data, &openTcpConnectionCommandData, sizeof(openTcpConnectionCommandData));
	atCommandPacket.atCommand = MODEM_COMMAND_OPEN_TCP_CONNECTION;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	timeoutMs -= (modem_interface_get_time_ms() - startTime);

	if (atResponsePacket.atResponse == MODEM_OK)
	{
		while (true)
		{
			if (ModemGetTcpConnectedState())
			{
				break;
			}

			modem_interface_task_delay(500UL);
			if (modem_interface_get_time_ms() > startTime + timeoutMs)
			{
				return MODEM_TIMEOUT;
			}
		}
	}
	else
	{
		return atResponsePacket.atResponse;
	}

	return MODEM_OK;
}

static void ServerOpenTcpConnection(uint32_t timeoutMs)
{
	OpenTcpConnectionCommandData_t openTcpConnectionCommandData;
	char atCommandBuf[MODEM_MAX_AT_COMMAND_SIZE + 1];
	char portBuf[6];

	memcpy(&openTcpConnectionCommandData, atCommandPacket.data, sizeof(openTcpConnectionCommandData));
	itoa(openTcpConnectionCommandData.port, portBuf, 10);
	strcpy(atCommandBuf, "AT+CIPSTART=\"TCP\",\"");
	strcat(atCommandBuf, openTcpConnectionCommandData.url);
	strcat(atCommandBuf, "\",\"");
	strcat(atCommandBuf, portBuf);
	strcat(atCommandBuf, "\"\r");

	atResponsePacket.atResponse = ServerSendBasicCommandResponse(atCommandBuf, timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// close TCP connection client and server functions

ModemStatus_t ModemCloseTcpConnection(uint32_t timeoutMs)
{
	uint32_t startTime = modem_interface_get_time_ms();
	ModemStatus_t modemStatus;

	modemStatus = ClientSendBasicCommandResponse(MODEM_COMMAND_CLOSE_TCP_CONNECTION, timeoutMs);
	if (modemStatus != MODEM_OK)
	{
		return modemStatus;
	}
	timeoutMs -= (modem_interface_get_time_ms() - startTime);

	while (ModemGetTcpConnectedState())
	{
		modem_interface_task_delay(500UL);
		if (modem_interface_get_time_ms() > startTime + timeoutMs)
		{
			return MODEM_TIMEOUT;
		}
	}

	return MODEM_OK;
}

static void ServerCloseTcpConnection(uint32_t timeoutMs)
{
	atResponsePacket.atResponse = ServerSendBasicCommandResponse("AT+CIPCLOSE", timeoutMs);
	if (atResponsePacket.atResponse == MODEM_CLOSE_OK)
	{
		tcpConnectedState = false;
	}
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// get operator details client and server functions

typedef struct
{
	char operatorDetails[MODEM_MAX_OPERATOR_DETAILS_LENGTH + 1];
} GetOperatorDetailsResponseData_t;

ModemStatus_t ModemGetOperatorDetails(char *operatorDetails, size_t length, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	GetOperatorDetailsResponseData_t getOperatorDetailsResponseData;

	if (operatorDetails == NULL || length < MODEM_MAX_OPERATOR_DETAILS_LENGTH + 1)
	{
		return MODEM_BAD_PARAMETER;
	}

	atCommandPacket.atCommand = MODEM_COMMAND_GET_OPERATOR_DETAILS;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}	

	memcpy(&getOperatorDetailsResponseData, atResponsePacket.data, sizeof(getOperatorDetailsResponseData));
	
	if (strncmp(getOperatorDetailsResponseData.operatorDetails, "+COPS: ", 	(size_t)7) == 0)
	{
		if (strlen(getOperatorDetailsResponseData.operatorDetails) > (size_t)7)
		{
			strcpy(operatorDetails, getOperatorDetailsResponseData.operatorDetails + 7);
		}
		else
		{
			atResponsePacket.atResponse = MODEM_UNEXPECTED_RESPONSE;
		}		
	}
	else
	{
		atResponsePacket.atResponse = MODEM_UNEXPECTED_RESPONSE;
	}

	return atResponsePacket.atResponse;
}

static void ServerGetOperatorDetails(uint32_t timeoutMs)
{
	GetOperatorDetailsResponseData_t getOperatorDetailsResponseData;

	atResponsePacket.atResponse = ServerSendBasicCommandTextResponse("AT+COPS?", getOperatorDetailsResponseData.operatorDetails, sizeof(getOperatorDetailsResponseData.operatorDetails), timeoutMs);
	if (atResponsePacket.atResponse == MODEM_OK)
	{
		if (strlen(getOperatorDetailsResponseData.operatorDetails) > (size_t)0)
		{
			getOperatorDetailsResponseData.operatorDetails[strlen(getOperatorDetailsResponseData.operatorDetails) - 1] = '\0';
		}
		memcpy(atResponsePacket.data, &getOperatorDetailsResponseData, sizeof(getOperatorDetailsResponseData));
	}
	
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// get own ip address client and server functions

typedef struct
{
	char ipAddress[MODEM_MAX_IP_ADDRESS_LENGTH + 1];
} GetOwnIpAddressResponseData_t;

ModemStatus_t ModemGetOwnIpAddress(char *ipAddress, size_t length, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	GetOwnIpAddressResponseData_t getOwnIpAddressResponseData;

	if (ipAddress == NULL || length < MODEM_MAX_IP_ADDRESS_LENGTH + 1)
	{
		return MODEM_BAD_PARAMETER;
	}

	atCommandPacket.atCommand = MODEM_COMMAND_GET_OWN_IP_ADDRESS;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	memcpy(&getOwnIpAddressResponseData, atResponsePacket.data, sizeof(getOwnIpAddressResponseData));
	strcpy(ipAddress, getOwnIpAddressResponseData.ipAddress);

	return atResponsePacket.atResponse;
}

static void ServerGetOwnIpAddress(uint32_t timeoutMs)
{
	GetOwnIpAddressResponseData_t getOwnIpAddressResponseData;
	uint32_t i = 0UL;
	uint32_t startTime = modem_interface_get_time_ms();
	ModemStatus_t modemStatus;

	modem_interface_serial_write_data((size_t)9, (uint8_t *)"AT+CIFSR\r");

	modemStatus = ServerGetEcho("AT+CIFSR", timeoutMs);
	if (modemStatus == MODEM_OK)
	{
		while (true)
		{
			if (modem_interface_serial_received_bytes_waiting() > (size_t)0)
			{
				modem_interface_serial_read_data((size_t)1, (uint8_t *)&getOwnIpAddressResponseData.ipAddress[i]);
				if (getOwnIpAddressResponseData.ipAddress[i] == '\n')
				{
					if (i >= 8UL)
					{
						getOwnIpAddressResponseData.ipAddress[i - 1UL] = '\0';
						memcpy(atResponsePacket.data, &getOwnIpAddressResponseData, sizeof(getOwnIpAddressResponseData));
						modemStatus = MODEM_OK;
					}
					else
					{
						modemStatus = MODEM_UNEXPECTED_RESPONSE;
					}
					break;
				}
				i++;

				if (i == MODEM_MAX_IP_ADDRESS_LENGTH)
				{
					modemStatus = MODEM_OVERFLOW;
					break;
				}
			}
			else
			{
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}
			}
		}
	}

	atResponsePacket.atResponse = modemStatus;
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// TCP write client and server functions

typedef struct
{
	uint8_t data[MODEM_MAX_TCP_WRITE_SIZE];
	size_t length;
} TcpWriteCommandData_t;

ModemStatus_t ModemTcpWrite(const uint8_t *data, size_t length, uint32_t timeoutMs)
{
	ModemStatus_t modemStatus = MODEM_OK;
	uint32_t startTime = modem_interface_get_time_ms();
	size_t lengthWritten = (size_t)0;
	size_t sectionLengthToWrite;

	if (length == (size_t)0)
	{
		return MODEM_OK;
	}

	if (data == NULL)
	{
		return MODEM_BAD_PARAMETER;
	}

	while (lengthWritten < length)
	{
		if (length >= (size_t)MODEM_MAX_TCP_WRITE_SIZE)
		{
			sectionLengthToWrite = MODEM_MAX_TCP_WRITE_SIZE;
		}
		else
		{
			sectionLengthToWrite = length;
		}

		modemStatus = ClientTcpWriteSection(data + (unsigned int)lengthWritten, sectionLengthToWrite, timeoutMs);
		if (modemStatus != MODEM_SEND_OK)
		{
			break;
		}

		lengthWritten += sectionLengthToWrite;
		timeoutMs -= (modem_interface_get_time_ms() - startTime);
	}

	return modemStatus;
}

static ModemStatus_t ClientTcpWriteSection(const uint8_t *data, size_t length, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	TcpWriteCommandData_t tcpWriteCommandData;

	memcpy(tcpWriteCommandData.data, data, length);
	tcpWriteCommandData.length = length;

	memcpy(atCommandPacket.data, &tcpWriteCommandData, sizeof(tcpWriteCommandData));
	atCommandPacket.atCommand = MODEM_COMMAND_TCP_WRITE;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	return atResponsePacket.atResponse;
}

static void ServerTcpWrite(uint32_t timeoutMs)
{
	TcpWriteCommandData_t tcpWriteCommandData;
	char atCommandBuf[MODEM_MAX_AT_COMMAND_SIZE + 1];
	char lengthBuf[6];
	char prompt[7];
	ModemStatus_t modemStatus;
	uint32_t startTime = modem_interface_get_time_ms();
	size_t i = (size_t)0;
	uint8_t dummy;

	memcpy(&tcpWriteCommandData, atCommandPacket.data, sizeof(tcpWriteCommandData));
	itoa(tcpWriteCommandData.length, lengthBuf, 10);
	strcpy(atCommandBuf, "AT+CIPSEND=");
	strcat(atCommandBuf, lengthBuf);
	strcat(atCommandBuf, "\r");

	modem_interface_serial_write_data(strlen(atCommandBuf), (uint8_t *)atCommandBuf);

	modemStatus = ServerGetEcho(atCommandBuf, timeoutMs);

	// the response from the modem can either be a prompt '> ' or the string 'ERROR\r\n'
	size_t promptExpectedLength = (size_t)2;
	size_t promptNextPos = (size_t)0;
	if (modemStatus == MODEM_OK)
	{
		while (true)
		{
			if (modem_interface_serial_received_bytes_waiting() >= (size_t)1)
			{
				modem_interface_serial_read_data((size_t)1, (uint8_t *)&prompt[promptNextPos]);
				if (prompt[promptNextPos] == 'E')
				{
					promptExpectedLength += (size_t)5;
				}
				promptNextPos++;
				if (promptNextPos == promptExpectedLength)
				{
					if (promptExpectedLength == (size_t)2)
					{
						if (memcmp(prompt, "> ", (size_t)2) == 0)
						{
							modemStatus = MODEM_OK;
						}
					}
					else
					{
						if (memcmp(prompt, "ERROR\r\n", (size_t)7) == 0)
						{
							modemStatus = MODEM_ERROR;
						}
						else
						{
							modemStatus = MODEM_UNEXPECTED_RESPONSE;
						}
					}
					break;
				}
			}
			else
			{
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}
			}
		}
	}

	if (modemStatus == MODEM_OK)
	{
		modem_interface_serial_write_data((uint16_t)tcpWriteCommandData.length, tcpWriteCommandData.data);
		while (i < tcpWriteCommandData.length)
		{
			if (modem_interface_serial_read_data((size_t)1, &dummy) == (size_t)1)
			{
				i++;
			}
			else
			{
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}
			}
		}
	}

	if (modemStatus == MODEM_OK)
	{
		modemStatus = ServerGetStandardResponse(timeoutMs);
	}
	
	if (modemStatus == MODEM_CLOSED)
	{
		tcpConnectedState = false;
	}

	atResponsePacket.atResponse = modemStatus;
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// get tcp read data waiting length client and server functions

typedef struct
{
	size_t length;
} GetTcpReadDataWaitinghResponseData_t;

ModemStatus_t ModemGetTcpReadDataWaitingLength(size_t *length, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	GetTcpReadDataWaitinghResponseData_t getTcpReadDataWaitinghResponseData;

	if (length == NULL)
	{
		return MODEM_BAD_PARAMETER;
	}

	atCommandPacket.atCommand = MODEM_COMMAND_GET_TCP_READ_DATA_WAITING_LENGTH;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	memcpy(&getTcpReadDataWaitinghResponseData, atResponsePacket.data, sizeof(getTcpReadDataWaitinghResponseData));
	*length = getTcpReadDataWaitinghResponseData.length;

	return atResponsePacket.atResponse;
}

static void ServerGetTcpReadDataWaitingLength(uint32_t timeoutMs)
{
	GetTcpReadDataWaitinghResponseData_t getTcpReadDataWaitinghResponseData;
	char responseText[25];
	char *dummy;

	atResponsePacket.atResponse = ServerSendBasicCommandTextResponse("AT+CIPRXGET=4", responseText, sizeof(responseText), timeoutMs);
	if (atResponsePacket.atResponse == MODEM_OK)
	{
		if (memcmp(responseText, "+CIPRXGET: 4,", (size_t)13) != 0)
		{
			atResponsePacket.atResponse = MODEM_UNEXPECTED_RESPONSE;
		}
		else
		{
			getTcpReadDataWaitinghResponseData.length = (uint16_t)strtol(responseText + 13, &dummy, 10);
			memcpy(atResponsePacket.data, &getTcpReadDataWaitinghResponseData, sizeof(getTcpReadDataWaitinghResponseData));
		}
	}
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// SMS receive message client and server functions

typedef struct
{
	uint8_t smsId;
} SmsReceiveCommandData_t;

typedef struct
{
	size_t length;
	uint8_t data[MODEM_SMS_MAX_PDU_LENGTH_ASCII_HEX + 1];
} SmsReadResponseData_t;

ModemStatus_t ModemSmsReceiveMessage(uint8_t smsId, size_t *lengthRead, uint8_t *buffer, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	SmsReceiveCommandData_t smsReceiveCommandData;
	SmsReadResponseData_t smsReceiveResponseData;
	
	if (lengthRead == NULL || buffer == NULL)
	{
		return MODEM_BAD_PARAMETER;
	}	
	
	*lengthRead = (size_t)0;
	smsReceiveCommandData.smsId = smsId;
	
	memcpy(atCommandPacket.data, &smsReceiveCommandData, sizeof(smsReceiveCommandData));
	atCommandPacket.atCommand = MODEM_COMMAND_SMS_RECEIVE_MESSAGE;
	atCommandPacket.timeoutMs = timeoutMs;	

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	
	memcpy(&smsReceiveResponseData, atResponsePacket.data, sizeof(smsReceiveResponseData));
	*lengthRead = smsReceiveResponseData.length;
	memcpy(buffer, smsReceiveResponseData.data, smsReceiveResponseData.length);	

	return atResponsePacket.atResponse;	
}

static void ServerSmsReceiveMessage(uint32_t timeoutMs)
{
	SmsReceiveCommandData_t smsReceiveCommandData;
	SmsReadResponseData_t smsReadResponseData;
	char commandText[25];
	char responseText[25];
	char numberBuf[5];
	ModemStatus_t modemStatus;
	size_t i = (size_t)0;
	uint32_t startTime = modem_interface_get_time_ms();
		
	memcpy(&smsReceiveCommandData, atCommandPacket.data, sizeof(smsReceiveCommandData));

	strcpy(commandText, "AT+CMGR=");
	itoa(smsReceiveCommandData.smsId, numberBuf, 10);
	strcat(commandText, numberBuf);
	strcat(commandText, ",0\r");	
	
	modem_interface_serial_write_data(strlen(commandText), (uint8_t *)commandText);

	modemStatus = ServerGetEcho(commandText, timeoutMs);
	if (modemStatus != MODEM_OK)
	{
		modemStatus = MODEM_TIMEOUT;
	}

	if (modemStatus == MODEM_OK)
	{	
		while (true)
		{
			if (modem_interface_serial_received_bytes_waiting() > (size_t)0)
			{
				modem_interface_serial_read_data((size_t)1, (uint8_t *)&responseText[i]);
				if (responseText[i] == '\n')
				{
					modemStatus = MODEM_OK;
					break;
				}
				i++;

				if (i == sizeof(responseText))
				{
					modemStatus = MODEM_OVERFLOW;
					break;
				}
			}
			else
			{
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}
			}
		}
	}
	
	if (modemStatus == MODEM_OK)
	{
		if (memcmp(responseText, "+CMGR: ", (size_t)7) != 0)
		{
			modemStatus = MODEM_UNEXPECTED_RESPONSE;
		}
	}	
	
	if (modemStatus == MODEM_OK)
	{	
		i = (size_t)0;
		while (true)
		{
			if (modem_interface_serial_received_bytes_waiting() > (size_t)0)
			{
				modem_interface_serial_read_data((size_t)1, (uint8_t *)&smsReadResponseData.data[i]);
				if (smsReadResponseData.data[i] == '\n')
				{
					modemStatus = MODEM_OK;
					break;
				}
				i++;

				if (i == sizeof(smsReadResponseData.data))
				{
					modemStatus = MODEM_OVERFLOW;
					break;
				}
			}
			else
			{
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}
			}
		}
		
		smsReadResponseData.length = i - (size_t)1;
		memcpy(atResponsePacket.data, &smsReadResponseData, sizeof(smsReadResponseData));		
	}

	if (modemStatus == MODEM_OK)
	{
		modemStatus = ServerGetStandardResponse(timeoutMs);
	}

	atResponsePacket.atResponse = modemStatus;
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);	
}

typedef struct
{
	char pdu[MODEM_SMS_MAX_PDU_LENGTH_ASCII_HEX + 1];
} SmsSendMessageCommandData_t;

ModemStatus_t ModemSmsSendMessage(char *buffer, uint32_t timeoutMs)
{
	SmsSendMessageCommandData_t smsSendMessageCommandData;
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;

	if (buffer == NULL)
	{
		return MODEM_BAD_PARAMETER;
	}

	strcpy(smsSendMessageCommandData.pdu, buffer);
	
	memcpy(atCommandPacket.data, &smsSendMessageCommandData, sizeof(smsSendMessageCommandData));
	atCommandPacket.atCommand = MODEM_COMMAND_SMS_SEND_MESSAGE;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	return atResponsePacket.atResponse;
}

static void ServerSmsSendMessage(uint32_t timeoutMs)
{
	SmsSendMessageCommandData_t smsSendMessageCommandData;
	char atCommandBuf[MODEM_MAX_AT_COMMAND_SIZE + 1];
	const char ctrlz[] = {26, '\0'};
	uint32_t startTime = modem_interface_get_time_ms();
	size_t i = (size_t)0;
	uint8_t dummy;
	char lengthBuf[6];
	ModemStatus_t modemStatus;
	char prompt[7];
	uint8_t write_response[12];
	
	memcpy(&smsSendMessageCommandData, atCommandPacket.data, sizeof(smsSendMessageCommandData));
	itoa(strlen((char *)smsSendMessageCommandData.pdu) / 2 - 1, lengthBuf, 10);		// length of data - length of smsc (not supplied here so a single 0, hence -1)

	strcpy(atCommandBuf, "AT+CMGS=");
	strcat(atCommandBuf, lengthBuf);
	strcat(atCommandBuf, "\r");	
	
	modem_interface_serial_write_data(strlen(atCommandBuf), (uint8_t *)atCommandBuf);
	modemStatus = ServerGetEcho(atCommandBuf, timeoutMs);	
	
	// the response from the modem can either be a prompt '> ' or the string 'ERROR\r\n'
	size_t promptExpectedLength = (size_t)2;
	size_t promptNextPos = (size_t)0;
	if (modemStatus == MODEM_OK)
	{
		while (true)
		{
			if (modem_interface_serial_received_bytes_waiting() >= (size_t)1)
			{
				modem_interface_serial_read_data((size_t)1, (uint8_t *)&prompt[promptNextPos]);
				if (prompt[promptNextPos] == 'E')
				{
					promptExpectedLength += 5U;
				}
				promptNextPos++;
				if (promptNextPos == promptExpectedLength)
				{
					if (promptExpectedLength == (size_t)2)
					{
						if (memcmp(prompt, "> ", (size_t)2) == 0)
						{
							modemStatus = MODEM_OK;
						}
					}
					else
					{
						if (memcmp(prompt, "ERROR\r\n", (size_t)7) == 0)
						{						
							modemStatus = MODEM_ERROR;
						}
						else
						{
							modemStatus = MODEM_UNEXPECTED_RESPONSE;
						}
					}
					break;
				}
			}
			else
			{
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}
			}
		}
	}	
	
	if (modemStatus == MODEM_OK)
	{
		strcat(smsSendMessageCommandData.pdu, ctrlz);	
		modem_interface_serial_write_data(strlen(smsSendMessageCommandData.pdu), (uint8_t *)smsSendMessageCommandData.pdu);
		while (i < strlen(smsSendMessageCommandData.pdu))
		{
			if (modem_interface_serial_read_data((size_t)1, &dummy) == (size_t)1)
			{
				i++;
			}
			else
			{
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}
			}
		}
	}	

	if (modemStatus == MODEM_OK)
	{
		while (true)
		{
			if (modem_interface_serial_received_bytes_waiting() >= (size_t)1)
			{
				modem_interface_serial_read_data((size_t)1, &dummy);	
				if (dummy == '\n')
				{
					break;
				}
				
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}				
			}
		}
	}

	// get and ignore +CMGS: xx\r\n response
	if (modemStatus == MODEM_OK)
	{
		i = 0;
		while (true)
		{
			if (modem_interface_serial_received_bytes_waiting() >= (size_t)1)
			{
				if (i == sizeof(write_response))
				{
					modemStatus = MODEM_UNEXPECTED_RESPONSE;
					break;
				}
				modem_interface_serial_read_data((size_t)1, &write_response[i]);	
				if (write_response[i] == '\n')
				{
					if (strncmp((char *)write_response, "+CMGS: ", 7) == 0)
					{
						modemStatus = MODEM_OK;
					}
					else if (strncmp((char *)write_response, "ERROR\r\n", 7) == 0)
					{
						modemStatus = MODEM_ERROR;
					}					
					else
					{
						modemStatus = MODEM_UNEXPECTED_RESPONSE;						
					}
					break;
				}
				
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}		

				i++;
			}
		}
	}	

	if (modemStatus == MODEM_OK)
	{
		modemStatus = ServerGetStandardResponse(timeoutMs);
	}	

	atResponsePacket.atResponse = modemStatus;
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// sms delete all messages client and server functions

ModemStatus_t ModemSmsDeleteAllMessages(uint32_t timeoutMs)
{
	return ClientSendBasicCommandResponse(MODEM_COMMAND_SMS_DELETE_ALL_MESSAGEs, timeoutMs);
}

static void ServerSmsDeleteAllMessages(uint32_t timeoutMs)
{
	atResponsePacket.atResponse = ServerSendBasicCommandResponse("AT+CMGD=1,4", timeoutMs);
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// tcp read client and server functions

typedef struct
{
	size_t lengthToRead;
} TcpReadCommandData_t;

typedef struct
{
	size_t lengthRead;
	uint8_t data[MODEM_MAX_TCP_READ_SIZE];
} TcpReadResponseData_t;

ModemStatus_t ModemTcpRead(size_t lengthToRead, size_t *lengthRead, uint8_t *buffer, uint32_t timeoutMs)
{
	ModemStatus_t modemStatus = MODEM_OK;
	uint32_t startTime = modem_interface_get_time_ms();
	size_t sectionLengthRead;
	size_t sectionLengthToRead;

	if (lengthRead == NULL || buffer == NULL)
	{
		return MODEM_BAD_PARAMETER;
	}

	if (lengthToRead == (size_t)0)
	{
		*lengthRead = (size_t)0;
		return MODEM_OK;
	}

	*lengthRead = (size_t)0;
	while (*lengthRead < lengthToRead)
	{
		if (lengthToRead >= (size_t)MODEM_MAX_TCP_READ_SIZE)
		{
			sectionLengthToRead = MODEM_MAX_TCP_READ_SIZE;
		}
		else
		{
			sectionLengthToRead = (size_t)lengthToRead;
		}
		modemStatus = ClientTcpReadSection(sectionLengthToRead, &sectionLengthRead, buffer + *lengthRead, timeoutMs);

		if (modemStatus != MODEM_OK)
		{
			break;
		}

		*lengthRead += (size_t)sectionLengthRead;
		timeoutMs -= (modem_interface_get_time_ms() - startTime);
	}

	return modemStatus;
}

static ModemStatus_t ClientTcpReadSection(size_t lengthToRead, size_t *lengthRead, uint8_t *buffer, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	TcpReadCommandData_t tcpReadCommandData;
	TcpReadResponseData_t tcpReadResponseData;

	tcpReadCommandData.lengthToRead = lengthToRead;
	memcpy(atCommandPacket.data, &tcpReadCommandData, sizeof(tcpReadCommandData));

	atCommandPacket.atCommand = MODEM_COMMAND_TCP_READ;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	memcpy(&tcpReadResponseData, atResponsePacket.data, sizeof(tcpReadResponseData));
	*lengthRead = tcpReadResponseData.lengthRead;
	memcpy(buffer, tcpReadResponseData.data, tcpReadResponseData.lengthRead);

	return atResponsePacket.atResponse;
}

static void ServerTcpRead(uint32_t timeoutMs)
{
	TcpReadCommandData_t tcpReadCommandData;
	TcpReadResponseData_t tcpReadResponseData;
	char commandText[25];
	char responseText[25];
	char numberBuf[5];
	ModemStatus_t modemStatus;
	size_t i = (size_t)0;
	uint32_t startTime = modem_interface_get_time_ms();

	memcpy(&tcpReadCommandData, atCommandPacket.data, sizeof(tcpReadCommandData));

	strcpy(commandText, "AT+CIPRXGET=2,");
	itoa(tcpReadCommandData.lengthToRead, numberBuf, 10);
	strcat(commandText, numberBuf);
	strcat(commandText, "\r");

	modem_interface_serial_write_data(strlen(commandText), (uint8_t *)commandText);

	modemStatus = ServerGetEcho(commandText, timeoutMs);
	if (modemStatus != MODEM_OK)
	{
		modemStatus = MODEM_TIMEOUT;
	}

	if (modemStatus == MODEM_OK)
	{
		while (true)
		{
			if (modem_interface_serial_received_bytes_waiting() > (size_t)0)
			{
				modem_interface_serial_read_data((size_t)1, (uint8_t *)&responseText[i]);
				if (responseText[i] == '\n')
				{
					modemStatus = MODEM_OK;
					break;
				}
				i++;

				if (i == sizeof(responseText))
				{
					modemStatus = MODEM_OVERFLOW;
					break;
				}
			}
			else
			{
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}
			}
		}
	}

	if (modemStatus == MODEM_OK)
	{
		char *dummy;

		if (memcmp(responseText, "+CIPRXGET: 2,", (size_t)13) != 0)
		{
			modemStatus = MODEM_UNEXPECTED_RESPONSE;
		}
		else
		{
			tcpReadResponseData.lengthRead = (uint16_t)strtol(responseText + 13UL, &dummy, 10);
		}
	}

	if (modemStatus == MODEM_OK)
	{
		i = 0U;
		while (i < tcpReadResponseData.lengthRead)
		{
			if (modem_interface_serial_read_data((size_t)1, &tcpReadResponseData.data[i]) > 0U)
			{
				i++;
			}
			else
			{
				if (modem_interface_get_time_ms() > startTime + timeoutMs)
				{
					modemStatus = MODEM_TIMEOUT;
					break;
				}
			}
		}

		memcpy(atResponsePacket.data, &tcpReadResponseData, sizeof(tcpReadResponseData));
	}

	if (modemStatus == MODEM_OK)
	{
		modemStatus = ServerGetStandardResponse(timeoutMs);
	}

	atResponsePacket.atResponse = modemStatus;
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

// get imei client and server functions

typedef struct
{
	char imei[MODEM_MAX_IMEI_LENGTH + 1];
} GetImeiResponseData_t;

ModemStatus_t ModemGetIMEI(char *imei, size_t length, uint32_t timeoutMs)
{
	AtCommandPacket_t atCommandPacket;
	AtResponsePacket_t atResponsePacket;
	GetImeiResponseData_t getImeiResponseData;

	if (imei == NULL || length < MODEM_MAX_IMEI_LENGTH + 1)
	{
		return MODEM_BAD_PARAMETER;
	}

	atCommandPacket.atCommand = MODEM_COMMAND_GET_IMEI;
	atCommandPacket.timeoutMs = timeoutMs;

	if (modem_interface_queue_put(MODEM_COMMAND_QUEUE, &atCommandPacket, 0UL) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}
	if (modem_interface_queue_get(MODEM_RESPONSE_QUEUE, &atResponsePacket, MODEM_INTERFACE_WAIT_FOREVER) != MODEM_OK)
	{
		return MODEM_FATAL_ERROR;
	}

	memcpy(&getImeiResponseData, atResponsePacket.data, sizeof(getImeiResponseData));
	strcpy(imei, getImeiResponseData.imei);

	return atResponsePacket.atResponse;
}

static void ServerGetImei(uint32_t timeoutMs)
{
	GetImeiResponseData_t getImeiResponseData;

	atResponsePacket.atResponse = ServerSendBasicCommandTextResponse("AT+GSN", getImeiResponseData.imei, sizeof(getImeiResponseData.imei), timeoutMs);
	if (atResponsePacket.atResponse == MODEM_OK)
	{
		if (strlen(getImeiResponseData.imei) > (size_t)0)
		{		
			getImeiResponseData.imei[strlen(getImeiResponseData.imei) - 1] = '\0';
		}
		memcpy(atResponsePacket.data, &getImeiResponseData, sizeof(getImeiResponseData));
	}
	
	ServerFlushReadBufferOnError(atResponsePacket.atResponse);
	modem_interface_queue_put(MODEM_RESPONSE_QUEUE, &atResponsePacket, 0UL);
}

const char *ModemStatusToText(ModemStatus_t modemStatus)
{
	switch (modemStatus)
	{
	case MODEM_OK:
		return "MODEM_OK";

	case MODEM_CLOSE_OK:
		return "MODEM_CLOSE_OK";

	case MODEM_SHUT_OK:
		return "MODEM_SHUT_OK";

	case MODEM_SEND_OK:
		return "MODEM_SEND_OK";

	case MODEM_ERROR:
		return "MODEM_ERROR";

	case MODEM_CLOSED:
		return "MODEM_CLOSED";

	case MODEM_TIMEOUT:
		return "MODEM_TIMEOUT";

	case MODEM_NO_RESPONSE:
		return "MODEM_NO_RESPONSE";

	case MODEM_UNEXPECTED_RESPONSE:
		return "MODEM_UNEXPECTED_RESPONSE";

	case MODEM_OVERFLOW:
		return "MODEM_OVERFLOW";

	case MODEM_BAD_PARAMETER:
		return "MODEM_BAD_PARAMETER";

	case MODEM_TCP_ALREADY_CONNECTED:
		return "MODEM_TCP_ALREADY_CONNECTED";

	case MODEM_FATAL_ERROR:
		return "MODEM_FATAL_ERROR";

	case MODEM_POWERED_DOWN:
		return "MODEM_POWERED_DOWN";

	default:
		return "MODEM_UNKNOWN_STATUS";
	}
}
