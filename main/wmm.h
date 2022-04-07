#ifndef WMM_H
#define WMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define WMM_EPOCH		2020.0f

typedef struct
{
	float gnm;
	float hnm;
	float dgnm;
	float dhnm;
} wmm_cof_record_t;

void E0000(float glat, float glon, float time_years, float *dec);
void wmm_init(void);
float wmm_get_date(uint8_t year, uint8_t month, uint8_t date);

#ifdef __cplusplus
}
#endif

#endif
