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

#ifndef INC_MODEM_H_
#define INC_MODEM_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************
*** INCLUDES ***
***************/

#include <stdbool.h>
#include <stdint.h>

/**************
*** DEFINES ***
**************/

#define MODEM_MAX_URC_LENGTH					50UL		///< Maximum length of URC accepted
#define MODEM_URC_TIMEOUT_MS					25UL		///< How long to wait for URC reception to finish after it has started in milliseconds
#define MODEM_SERVER_LOOP_PERIOD_MS				25UL		///< How long to delay in each loop of server task in milliseconds
#define MODEM_MAX_APN_LENGTH					20UL		///< Maximum allowed GSM data connection APN length
#define MODEM_MAX_USERNAME_LENGTH				12UL		///< Maximum allowed GSM data connection username length
#define MODEM_MAX_PASSWORD_LENGTH				12UL		///< Maximum allowed GSM data connection password length
#define MODEM_MAX_AT_COMMAND_SIZE				600UL		///< Maximum allowed AT command length 
#define MODEM_MAX_AT_RESPONSE_SIZE				600UL		///< Maximum allowed AT response length 
#define MODEM_MAX_URL_ADDRESS_SIZE				70L			///< Maximum alllowed URL length when opening TCP connection
#define MODEM_MAX_IP_ADDRESS_LENGTH				20L			///< Maximum alllowed IP address length in x.x.x.x format
#define MODEM_MAX_TCP_WRITE_SIZE				99UL		///< Maximum allowed TCP write per AT command size
#define MODEM_MAX_TCP_READ_SIZE					99UL		///< Maximum allowed TCP write per AT response size
#define MODEM_MAX_OPERATOR_DETAILS_LENGTH		50UL		///< Maximum allowed length of operator details when reading from modem
#define MODEM_SMS_MAX_TEXT_LENGTH				160UL		///< Maximum SMS message length
#define MODEM_SMS_MAX_PDU_LENGTH_BINARY			256UL		///< Maximum SMS PDU length in binary
#define MODEM_SMS_MAX_PDU_LENGTH_ASCII_HEX		(MODEM_SMS_MAX_PDU_LENGTH_BINARY * 2)		///< Maximum SMS PDU length in ascii hex
#define MODEM_MAX_IMEI_LENGTH					16UL		///< Maximum allowed length of IMEI when reading from modem
#define MODEM_MAX_PHONE_NUMBER_LENGTH			20UL		///< Maximum allowed length SMS sender's phone number

/************
*** TYPES ***
************/

/**
 * Enumeration of response status or errors from modem API
 */
typedef enum
{
	// ok statuses
	MODEM_OK = 0,						///< No error, command completed successfully
	MODEM_CLOSE_OK = 1,					///< CLOSE URC received without error
	MODEM_SHUT_OK = 2,					///< SHUT URC received without error
	MODEM_SEND_OK = 3,					///< TCP send completed successfully
	MODEM_CLOSED = 4,					///< CLOSED URC received without error
	MODEM_POWERED_DOWN = 5,				///< POWERED_DOWN URC received without error

	// error statuses
	MODEM_ERROR = -1,					///< An ERROR URC has been received
	MODEM_TIMEOUT = -2,					///< Operation did not complete within specified timeout time
	MODEM_NO_RESPONSE = -3,				///< No response was received when one was expected
	MODEM_UNEXPECTED_RESPONSE = -4,		///< An unexpected response was received
	MODEM_OVERFLOW = -5,				///< Not enough space in a buffer
	MODEM_BAD_PARAMETER = -6,			///< An API parameter is illegal
	MODEM_TCP_ALREADY_CONNECTED = -7,	///< TCP cannot connext as it's already connected
	MODEM_FATAL_ERROR = -8				///< An unspecified modem error has occurred that caused the current command to be abandoned
} ModemStatus_t;

typedef enum
{
	MODEM_COMMAND_HELLO,							///< Implementing modem command AT which is a test of modem communications
	MODEM_COMMAND_NETWORK_REGISTRATION,				///< Implementing modem command AT+CREG? which reads network registration status
	MODEM_COMMAND_SIGNAL_STRENGTH,					///< Implementing modem command AT+CSQ which reads signal strength
	MODEM_COMMAND_SET_MANUAL_DATA_READ,				///< Implementing modem command AT+CIPRXGET (mode = 1) which sets manual TCP data read
	MODEM_COMMAND_CONFIGURE_DATA_CONNECTION,		///< Implementing modem command AT+CSTT which configures the GPRS data connection
	MODEM_COMMAND_ACTIVATE_DATA_CONNECTION,			///< Implementing modem command AT+CIICR which activates the GPRS data connection
	MODEM_COMMAND_GET_OWN_IP_ADDRESS,				///< Implementing modem command AT+CIFSR which reads the IP address assigned
	MODEM_COMMAND_OPEN_TCP_CONNECTION,				///< Implementing modem command AT+CIPSTART which opens a TCP connection
	MODEM_COMMAND_TCP_WRITE,						///< Implementing modem command AT+CIPSEND which writes to a TCP connetion
	MODEM_COMMAND_GET_TCP_READ_DATA_WAITING_LENGTH,	///< Implementing modem command AT+CIPRXGET (mode = 4) which reads how many TCP bytes received waiting to be read
	MODEM_COMMAND_TCP_READ,							///< Implementing modem command AT+CIPRXGET (mode = 2) which reads received TCP data
	MODEM_COMMAND_CLOSE_TCP_CONNECTION,				///< Implementing modem command AT+CIPCLOSE which closes a TCP conneciton
	MODEM_COMMAND_DEACTIVATE_DATA_CONNECTION,		///< Implementing modem command AT+CIPSHUT which deactivates the data connection
	MODEM_COMMAND_SET_SMS_PDU_MODE,					///< Implementing modem command AT+CMGF which sets SMS PDU mode
	MODEM_COMMAND_SET_SMS_RECEIVE_MODE,				///< Implementing modem command AT+CNMI	which sets SMS receive mode to URC notification
	MODEM_COMMAND_SMS_RECEIVE_MESSAGE,				///< Implementing modem command AT+CMGR	which reads a received SMS message
	MODEM_COMMAND_SMS_SEND_MESSAGE,				  	///< Implementing modem command AT+CMGS	which sends a SMS message	
	MODEM_COMMAND_SMS_DELETE_ALL_MESSAGEs,			///< Implementing modem command AT+CMGD	which deletes all SMS messages on the modem
	MODEM_COMMAND_POWER_DOWN,						///< Implementing modem command AT+CPOWD which powers down the moded
	MODEM_COMMAND_GET_OPERATOR_DETAILS,				///< Implementing modem command AT+COPS	which reads the operator details
	MODEM_COMMAND_GET_IMEI							///< Implementing modem command AT+GSN which reads the modem's IMEI
} AtCommand_t;

/**
 * Callback function type declaration for when a new SMS message arrives
 *
 * @param smsId The identifier of the message strored in the modem 
 */
typedef void (*SmsNotificationCallback_t)(uint32_t smsId);

/**
 * Struct of a AT command packet sent from the client side to the server side of the modem driver before the server side code builds the AT command and sends it to the modem
 */ 
typedef struct
{
	uint32_t timeoutMs;								///< Timeout in milliseconds for the command to complete
	AtCommand_t atCommand;							///< Enum value identifyingt the AT command
	uint8_t data[MODEM_MAX_AT_COMMAND_SIZE];		///< Packaged up data of a struct containing values used to build up the AT command
} AtCommandPacket_t;

/**
 * Struct of a AT response packet sent from the server side to the client side of the modem driver after the server side has received the response to an AT command
 */ 
typedef struct
{
	ModemStatus_t atResponse;						///< Enum of response status
	uint8_t data[MODEM_MAX_AT_RESPONSE_SIZE];		///< Packaged up data of a struct containing values received in the AT command response
} AtResponsePacket_t;

/*************************
*** EXTERNAL VARIABLES ***
*************************/

/***************************
*** FUNCTIONS PROTOTYPES ***
***************************/

/**
 * Run the modem task called from the modem interface layer allowing the task function to be operating system agnostic
 */
void DoModemTask(void);

/**
 * Reset the modem using an AT command
 */
void ModemReset(void);

/**
 * Delete the modem, all its allocated data and delete all its operating system objects and close hardware connections
 * 
 * @note The actual deleteing and closing is done in the modem interface layer
 */
void ModemDelete(void);

/**
 * Set a callback function to be called when an incoming SMS URC is received
 *
 * @param smsNotificationCallback The function to be called
 * @return A status or error code
 */
ModemStatus_t ModemSetSmsNotificationCallback(SmsNotificationCallback_t smsNotificationCallback);

/**
 * Call this once before using other API functions to initialize the modem, open hardware connections and create operating system objects
 *
 * @note This function sets AT command echoing on
 * @return A status or error code
 */ 
ModemStatus_t ModemInit(void);

/**
 * Send a modem bare AT command and ger response to confirm that the AT interface is working
 *
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */  
ModemStatus_t ModemHello(uint32_t timeoutMs);

/**
 * Set the modem SMS mode to PDU
 *
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */  
ModemStatus_t ModemSetSmsPduMode(uint32_t timeoutMs);

/**
 * Set the modem SMS reception mode to URC notification
 *
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */  
ModemStatus_t ModemSetSmsReceiveMode(uint32_t timeoutMs);

/**
 * Read a received SMS message complete PDU
 *
 * @param smsId The message id as received in the new message callback
 * @param lengthRead Pointer to variable to hold the length of the message PDU
 * @param buffer Buffer to hold PDU in ascii hex format of at least (SMS_MAX_PDU_LENGTH * 2 + 1) bytes length
 * @param bufferLength Length of buffer in bytes
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */  
ModemStatus_t ModemSmsReceiveMessage(uint8_t smsId, size_t *lengthRead, uint8_t *buffer, size_t bufferLength, uint32_t timeoutMs);

/**
 * Send a SMS message complete PDU
 *
 * @param buffer String holding the PDU in ascii hex format
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */  
ModemStatus_t ModemSmsSendMessage(const char *buffer, uint32_t timeoutMs);

/**
 * Delete all SMS messages on the modem in all storages
 *
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */  
ModemStatus_t ModemSmsDeleteAllMessages(uint32_t timeoutMs);

/**
 * Get network registered status
 *
 * @param registered Pointer to variable to hold registered status
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemGetNetworkRegistrationStatus(bool *registered, uint32_t timeoutMs);

/**
 * Get signal strength
 *
 * @param strength Pointer to variable to hold signal strength, 1-31 or 99 meaning unavailable
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemGetSignalStrength(uint8_t *strength, uint32_t timeoutMs);

/**
 * Get operator details
 *
 * @param operatorDetails Pointer to string buffer to hold operator details 
 * @param length Size of operatorDetails in bytes
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemGetOperatorDetails(char *operatorDetails, size_t length, uint32_t timeoutMs);

/**
 * Set TCP received data read to manual
 *
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemSetManualDataRead(uint32_t timeoutMs);

/**
 * Configure at IP data connectiuon
 *
 * @param apn Pointer to string of the Access Point Name
 * @param username Pointer to string of the username
 * @param password Pointer to string of the password
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemConfigureDataConnection(const char *apn, const char *username, const char *password, uint32_t timeoutMs);

/**
 * Activate the modem's data connection
 *
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemActivateDataConnection(uint32_t timeoutMs);

/**
 * Get own IP address details
 *
 * @param ipAddress Pointer to string buffer to hold IP address
 * @param length Size of ipAddress in bytes
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemGetOwnIpAddress(char *ipAddress, size_t length, uint32_t timeoutMs);

/**
 * Open a TCP connection
 *
 * @param url The URL of the remote device to connect to
 * @param port The IP port to use
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemOpenTcpConnection(const char *url, uint16_t port, uint32_t timeoutMs);

/**
 * Write to an opened TCP connection
 *
 * @param data The data to write
 * @param length The number of bytes to write
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemTcpWrite(const uint8_t *data, size_t length, uint32_t timeoutMs);

/**
 * Get received bytes via TCP waiting to be read
 *
 * @param length Pointer to variable to hold bytes waiting
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemGetTcpReadDataWaitingLength(size_t *length, uint32_t timeoutMs);

/**
 * Read received bytes from a TCP connection
 *
 * @param lengthToRead How many bytes to attempt to read
 * @param lengthRead How many bytes were read
 * @param buffer Buffer to place read bytes into
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemTcpRead(size_t lengthToRead, size_t *lengthRead, uint8_t *buffer, uint32_t timeoutMs);

/**
 * Close the TCP connection
 *
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemCloseTcpConnection(uint32_t timeoutMs);

/**
 * Deactivate a data connection
 *
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */
ModemStatus_t ModemDeactivateDataConnection(uint32_t timeoutMs);

/**
 * Power down the modem hardware
 *
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemPowerDown(uint32_t timeoutMs);

/**
 * Get IMEI details
 *
 * @param imei Pointer to string buffer to IMEI
 * @param length Size of imei in bytes
 * @param timeoutMs Time to wait in milliseconds for the command to complete
 * @return A status or error code
 */ 
ModemStatus_t ModemGetIMEI(char *imei, size_t length, uint32_t timeoutMs);

/**
 * Convert a modem status/error code to text
 *
 * @param modemStatus The status or error code
 * @return Text of the status or error code
 */
const char *ModemStatusToText(ModemStatus_t modemStatus);

/**
 * Get if there is a current TCP connection
 *
 * @return If there is a TCP connection true else false
 */
bool ModemGetTcpConnectedState(void);

/**
 * Get if there is a current data context active
 *
 * @return If there is a data context active true else false
 */
bool ModemGetPdpActivatedState(void);

#ifdef __cplusplus
}
#endif

#endif
