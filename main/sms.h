#ifndef SMS_H
#define SMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define SMS_MAX_PHONE_NUMBER_LENGTH		24UL

void sms_init(void);
bool new_sms_check(uint32_t *sms_id);
bool sms_receive(uint32_t sms_id, char *phone_number, size_t phone_number_buffer_length, char *message_text, size_t message_text_buffer_length);
bool sms_send(const char *message_text, const char *phone_number);

#ifdef __cplusplus
}
#endif

#endif