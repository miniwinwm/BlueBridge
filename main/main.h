#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

TaskHandle_t get_main_task_handle(void);

#ifdef __cplusplus
}
#endif

#endif
