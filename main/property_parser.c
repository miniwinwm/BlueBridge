#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "property_parser.h"

typedef enum 
{
	NOT_STARTED,
	IN_KEY,
	RECEIVED_EQUALS,
	IN_VALUE,
} parse_state_t;

uint16_t property_parse(char *str, parser_callback_t parser_callback)
{
	char *key = "";
	char *value = "";	
	uint16_t c;
	parse_state_t parse_state;
	uint16_t found = 0U;
	
	if (!str || !parser_callback)
	{
		return 0U;
	}

	size_t len = strlen(str);
	
	if (len == (size_t)0)
	{
		return 0U;
	}

	c = 0U;
	parse_state = NOT_STARTED;
	while (true)
	{
		if (c == len)
		{
			break;
		}
		
		if (parse_state == NOT_STARTED)
		{			
			if (str[c] == '=' || str[c] == '\r' || str[c] == '\n')
			{
				c++;				
				continue;
			}

			key = &str[c];
			parse_state = IN_KEY;
			c++;
			continue;
		}
		else if (parse_state == IN_KEY)
		{
			if (str[c] == '\r' || str[c] == '\n')
			{				
				parse_state = NOT_STARTED;
				str[c] = '\0';		

				if (strlen(key) > (size_t)0)
				{
					found++;
					parser_callback(key, "");
				}
				key = "";				
			}
			else if (str[c] == '=')
			{
				str[c] = '\0';				
				parse_state = RECEIVED_EQUALS;
			}
			c++;
			continue;
		}
		else if (parse_state == RECEIVED_EQUALS)
		{	
			if (str[c] == '\n' || str[c] == '\r')
			{
				key = "";
				parse_state = NOT_STARTED;
			}
			else
			{
				parse_state = IN_VALUE;
				value = &str[c];
			}
			c++;
			continue;
		}
		else if (parse_state == IN_VALUE)
		{		
			if (str[c] == '\r' || str[c] == '\n')
			{
				parse_state = NOT_STARTED;
				str[c] = '\0';
				
				if (strlen(key) > (size_t)0 && strlen(value) > (size_t)0)
				{
					found++;
					parser_callback(key, value);
				}
				
				key = "";
				value = "";
			}
			c++;
			continue;
		}
	}	
	
	if (strlen(key) > (size_t)0)
	{
		if (parser_callback(key, value))
		{
			found++;
		}
	}	
	
	return found;
}