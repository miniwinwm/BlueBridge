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
#include <ctype.h>
#include <stdio.h>
#include "util.h"

bool safe_strcat(char *dest, size_t size, const char *src)
{
    if (dest == NULL || src == NULL || (strlen(dest) + strlen(src) + (size_t)1 > size))
    {
    	return false;
    }

	return (strncat((dest), (src), (size - strlen(dest) - (size_t)1U)));
}

uint32_t util_htoi(const char *s)
{
    uint32_t val = 0UL;
    uint8_t x = 0U;
	
	if (s == NULL)
	{
		return 0UL;
	}
          
    while (s[x] != '\0')
    {
		if (s[x] >= '0' && s[x] <= '9')
		{
			val = val * 16UL + (uint32_t)(s[x] - '0');
		}
		else if (s[x] >= 'A' && s[x] <= 'F')
		{
			val = val * 16UL + (uint32_t)(s[x] - 'A') + 10UL;
		}
		else if (s[x] >= 'a' && s[x] <= 'f')
		{
			val = val * 16UL + (uint32_t)(s[x] - 'a') + 10UL;
		}
		else 
		{
			return 0UL;
		}
        
		x++;
    }
	
    return val;
}

void util_replace_char(char *s, char orig, char rep) 
{
    char *ix = s;
	
	if (s == NULL)
	{
		return;
	}
	
    while((ix = strchr(ix, orig))) 
	{
        *ix++ = rep;
    }
}

char *util_capitalize_string(char *s)
{
	char *o = s;
	
	if (s == NULL)
	{
		return NULL;
	}
		
	while ((*s = toupper(*s)))
	{
		s++;
	}

	return o;
}

char *util_seconds_to_hms(uint32_t seconds)
{
	static char result[15];
	char f[10];	
	uint32_t h = seconds / 3600UL;
	uint32_t m = (seconds - (seconds / 3600UL) * 3600UL ) / 60UL;
	uint32_t s = seconds % 60UL;
	
	result[0] = '\0';
	if (h > 0)
	{
		snprintf(f, sizeof(f), "%uh", (unsigned int)h);
		strcat(result, f);
	}
	
	if (m > 0UL)
	{
		snprintf(f, sizeof(f), "%um", (unsigned int)m);
		strcat(result, f);		
	}
	
	if (s > 0UL)
	{
		snprintf(f, sizeof(f), "%us", (unsigned int)s);
		strcat(result, f);		
	}	
	
	return result;
}

bool util_hms_to_seconds(const char *hms, uint32_t *result)
{
	uint32_t field_val = 0UL;
	
	if (result == NULL)
	{
		return false;
	}
	
	*result = 0UL;	
	
	if (hms == NULL)
	{
		return false;
	}
	
	if (strlen(hms) < (size_t)2)
	{
		return false;
	}
	
	if (hms[strlen(hms) - (size_t)1] != 's' && hms[strlen(hms) - (size_t)1] != 'm' && hms[strlen(hms) - (size_t)1] != 'h')
	{
		return false;
	}
		
	while (true)
	{
		if (*hms == '\0')
		{
			break;
		}
		
		if (isdigit(*hms))
		{
			field_val *= 10UL;
			field_val += *hms - '0';
		}
		else if (*hms == 'h')
		{
			*result += field_val * 3600UL;
			field_val = 0UL;
		}
		else if (*hms == 'm')
		{
			*result += field_val * 60UL;
			field_val = 0UL;		
		}		
		else if (*hms == 's')
		{			
			*result += field_val;
			field_val = 0UL;			
		}	
		else
		{
			return false;
		}
		
		hms++;
	}	
	
	return true;
}

uint32_t util_hash_djb2(const char *str)
{
	uint32_t hash = 5381UL;
	char c;

	while ((c = *str++))
	{
		hash = ((hash << 5) + hash) + (uint32_t)c; 
	}

	return hash;
}