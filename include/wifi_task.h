#include <FreeRTOS.h>
#include <semphr.h>

void  wifi_task(void *pvParameters);

extern xSemaphoreHandle wifi_alive;
extern xQueueHandle publish_queue;
