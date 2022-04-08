#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

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
	static char s[15];
	if (seconds < 60UL)
	{
		snprintf(s, sizeof(s), "%02us", (unsigned int)seconds);
	}
	else if (seconds < 3600UL)
	{
		snprintf(s, sizeof(s), "%02um%02us", (unsigned int)(seconds / 60UL), (unsigned int)(seconds % 60UL));		
	}
	else
	{
		snprintf(s, sizeof(s), "%uh%02um%02us", (unsigned int)(seconds / 3600UL), (unsigned int)((seconds - (seconds / 3600UL) * 3600UL ) / 60UL), (unsigned int)(seconds % 60UL));		
	}
	
	return s;
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
	
	if (hms[strlen(hms) - (size_t)1] != 's')
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

uint32_t util_hash_djb2(uint8_t *str)
{
	uint32_t hash = 5381UL;
	uint8_t c;

	while ((c = *str++))
	{
		hash = ((hash << 5) + hash) + (uint32_t)c; 
	}

	return hash;
}