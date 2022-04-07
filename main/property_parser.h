#ifndef PROPERTY_PARSER_H
#define PROPERTY_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*parser_callback_t)(char *key, char *value);

uint16_t property_parse(char *str, parser_callback_t parser_callback);

#ifdef __cplusplus
}
#endif

#endif