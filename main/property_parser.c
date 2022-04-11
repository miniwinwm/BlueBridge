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