#ifndef FLASH_H
#define FLASH_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stddef.h>

void flash_init(void);
void flash_load_data(void *data, size_t length);
void flash_store_data(void *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
