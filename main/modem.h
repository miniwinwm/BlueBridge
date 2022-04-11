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

#ifndef INC_MODEM_H_
#define INC_MODEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define MODEM_MAX_URC_LENGTH					50UL		// maximum length of URC accepted
#define MODEM_URC_TIMEOUT_MS					25UL		// how long to wait for URC reception to finish after it has started
#define MODEM_SERVER_LOOP_PERIOD_MS				25UL
#define MODEM_MAX_APN_LENGTH					20UL
#define MODEM_MAX_USERNAME_LENGTH				12UL
#define MODEM_MAX_PASSWORD_LENGTH				12UL
#define MODEM_MAX_AT_COMMAND_SIZE				600UL
#define MODEM_MAX_AT_RESPONSE_SIZE				600UL
#define MODEM_MAX_URL_ADDRESS_SIZE				70L
#define MODEM_MAX_IP_ADDRESS_LENGTH				20L
#define MODEM_MAX_TCP_WRITE_SIZE				99UL
#define MODEM_MAX_TCP_READ_SIZE					99UL
#define MODEM_MAX_OPERATOR_DETAILS_LENGTH		50UL
#define MODEM_SMS_MAX_TEXT_LENGTH				160UL
#define MODEM_SMS_MAX_PDU_LENGTH_BINARY			256UL
#define MODEM_SMS_MAX_PDU_LENGTH_ASCII_HEX		(MODEM_SMS_MAX_PDU_LENGTH_BINARY * 2)
#define MODEM_MAX_IMEI_LENGTH					16UL
#define MODEM_MAX_PHONE_NUMBER_LENGTH			20UL

typedef enum
{
	// ok statuses
	MODEM_OK = 0,
	MODEM_CLOSE_OK = 1,
	MODEM_SHUT_OK = 2,
	MODEM_SEND_OK = 3,
	MODEM_CLOSED = 4,
	MODEM_POWERED_DOWN = 5,

	// error statuses
	MODEM_ERROR = -1,
	MODEM_TIMEOUT = -2,
	MODEM_NO_RESPONSE = -3,
	MODEM_UNEXPECTED_RESPONSE = -4,
	MODEM_OVERFLOW = -5,
	MODEM_BAD_PARAMETER = -6,
	MODEM_TCP_ALREADY_CONNECTED = -7,
	MODEM_FATAL_ERROR = -8
} ModemStatus_t;

typedef enum
{
	MODEM_COMMAND_HELLO,							// AT
	MODEM_COMMAND_NETWORK_REGISTRATION,				// AT+CREG?
	MODEM_COMMAND_SIGNAL_STRENGTH,					// AT+CSQ
	MODEM_COMMAND_SET_MANUAL_DATA_READ,				// AT+CIPRXGET (mode = 1)
	MODEM_COMMAND_CONFIGURE_DATA_CONNECTION,		// AT+CSTT
	MODEM_COMMAND_ACTIVATE_DATA_CONNECTION,			// AT+CIICR
	MODEM_COMMAND_GET_OWN_IP_ADDRESS,				// AT+CIFSR
	MODEM_COMMAND_OPEN_TCP_CONNECTION,				// AT+CIPSTART
	MODEM_COMMAND_TCP_WRITE,						// AT+CIPSEND
	MODEM_COMMAND_GET_TCP_READ_DATA_WAITING_LENGTH,	// AT+CIPRXGET (mode = 4)
	MODEM_COMMAND_TCP_READ,							// AT+CIPRXGET (mode = 2)
	MODEM_COMMAND_CLOSE_TCP_CONNECTION,				// AT+CIPCLOSE
	MODEM_COMMAND_DEACTIVATE_DATA_CONNECTION,		// AT+CIPSHUT
	MODEM_COMMAND_SET_SMS_PDU_MODE,					// AT+CMGF
	MODEM_COMMAND_SET_SMS_RECEIVE_MODE,				// AT+CNMI	
	MODEM_COMMAND_SMS_RECEIVE_MESSAGE,				// AT+CMGR	
	MODEM_COMMAND_SMS_SEND_MESSAGE,				  	// AT+CMGS		
	MODEM_COMMAND_SMS_DELETE_ALL_MESSAGEs,			// AT+CMGD		
	MODEM_COMMAND_POWER_DOWN,						// AT+CPOWD
	MODEM_COMMAND_GET_OPERATOR_DETAILS,				// AT+COPS	
	MODEM_COMMAND_GET_IMEI							// AT+GSN		
} AtCommand_t;

typedef void (*SmsNotificationCallback_t)(uint32_t smsId);

typedef struct
{
	uint32_t timeoutMs;
	AtCommand_t atCommand;
	uint8_t data[MODEM_MAX_AT_COMMAND_SIZE];
} AtCommandPacket_t;

typedef struct
{
	ModemStatus_t atResponse;
	uint8_t data[MODEM_MAX_AT_RESPONSE_SIZE];
} AtResponsePacket_t;

void DoModemTask(void);
void ModemReset(void);
void ModemDelete(void);
ModemStatus_t ModemSetSmsNotificationCallback(SmsNotificationCallback_t smsNotificationCallback);
ModemStatus_t ModemInit(void);
ModemStatus_t ModemHello(uint32_t timeoutMs);
ModemStatus_t ModemSetSmsPduMode(uint32_t timeoutMs);
ModemStatus_t ModemSetSmsReceiveMode(uint32_t timeoutMs);
ModemStatus_t ModemSmsReceiveMessage(uint8_t smsId, size_t *lengthRead, uint8_t *buffer, uint32_t timeoutMs);
ModemStatus_t ModemSmsSendMessage(char *buffer, uint32_t timeoutMs);
ModemStatus_t ModemSmsDeleteAllMessages(uint32_t timeoutMs);
ModemStatus_t ModemGetNetworkRegistrationStatus(bool *registered, uint32_t timeoutMs);
ModemStatus_t ModemGetSignalStrength(uint8_t *strength, uint32_t timeoutMs);
ModemStatus_t ModemGetOperatorDetails(char *operatorDetails, size_t length, uint32_t timeoutMs);
ModemStatus_t ModemSetManualDataRead(uint32_t timeoutMs);
ModemStatus_t ModemConfigureDataConnection(const char *apn, const char *username, const char *password, uint32_t timeoutMs);
ModemStatus_t ModemActivateDataConnection(uint32_t timeoutMs);
ModemStatus_t ModemGetOwnIpAddress(char *ipAddress, size_t length, uint32_t timeoutMs);
ModemStatus_t ModemOpenTcpConnection(const char *url, uint16_t port, uint32_t timeoutMs);
ModemStatus_t ModemTcpWrite(const uint8_t *data, size_t length, uint32_t timeoutMs);
ModemStatus_t ModemGetTcpReadDataWaitingLength(size_t *length, uint32_t timeoutMs);
ModemStatus_t ModemTcpRead(size_t lengthToRead, size_t *lengthRead, uint8_t *buffer, uint32_t timeoutMs);
ModemStatus_t ModemCloseTcpConnection(uint32_t timeoutMs);
ModemStatus_t ModemDeactivateDataConnection(uint32_t timeoutMs);
ModemStatus_t ModemPowerDown(uint32_t timeoutMs);
ModemStatus_t ModemGetIMEI(char *imei, size_t length, uint32_t timeoutMs);
const char *ModemStatusToText(ModemStatus_t modemStatus);
bool ModemGetTcpConnectedState(void);
bool ModemGetPdpActivatedState(void);

#ifdef __cplusplus
}
#endif

#endif
