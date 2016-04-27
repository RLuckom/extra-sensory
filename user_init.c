#include "esp/uart.h"
#include <ssid_config.h>
#include "include/dht11.h"
#include "include/http_post.h"

void user_init(void)
{
    uart_set_baud(0, 115200);

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* required to call wifi_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    xTaskCreate(&http_post_task, (signed char *)"post_task", 512, NULL, 2, NULL);
    xTaskCreate(tempReadTask, (signed char *)"tempReadTask", 1048, NULL, 5, NULL);
}
