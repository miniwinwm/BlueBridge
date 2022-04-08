#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

uint32_t util_htoi(const char *s);
void util_replace_char(char *s, char orig, char rep);
char *util_capitalize_string(char *s);
char *util_seconds_to_hms(uint32_t seconds);
bool util_hms_to_seconds(const char *hms, uint32_t *result);
uint32_t util_hash_djb2(uint8_t *str);

#ifdef __cplusplus
}
#endif

#endif