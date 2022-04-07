#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

uint32_t htoi(const char *s);
void replace_char(char *s, char orig, char rep);
char *capitalize_string(char *s);
char *seconds_to_hms(uint32_t seconds);
bool hms_to_seconds(const char *hms, uint32_t *result);
uint32_t util_hash_djb2(uint8_t *str);

#ifdef __cplusplus
}
#endif

#endif