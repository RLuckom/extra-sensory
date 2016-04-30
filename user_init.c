#include <FreeRTOS.h>
#include <semphr.h>
#include "esp/uart.h"
#include <ssid_config.h>
#include "include/dht11.h"
#include "include/mqtt_client.h"
#include "include/wifi_task.h"


void user_init(void)
{
    uart_set_baud(0, 115200);

    vSemaphoreCreateBinary(wifi_alive);
    publish_queue = xQueueCreate(3, PUB_MSG_LEN);
    xTaskCreate(&wifi_task, (int8_t *)"wifi_task",  256, NULL, 2, NULL);
    xTaskCreate(tempReadTask, (signed char *)"tempReadTask", 1048, NULL, 5, NULL);
    xTaskCreate(&enqueue_task, (int8_t *)"enqueue", 256, NULL, 3, NULL);
    xTaskCreate(&mqtt_task, (int8_t *)"mqtt_task", 1024, NULL, 4, NULL);
}
