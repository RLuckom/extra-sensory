#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include <string.h>
#include "FreeRTOS.h"
#include <ssid_config.h>
#include "esp8266.h"

#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "../include/http_post.h"

#define WEB_SERVER "192.168.1.132"
#define WEB_PORT 3000
#define WEB_URL "http://192.168.1.132:3000/"
char request[300];
char details[80];
char headers[200];

extern float temperature;
extern float humidity;
extern float moisture;
extern float light;

void http_post_task(void *pvParameters)
{
    int successes = 0, failures = 0;
    printf("HTTP get task starting...\r\n");

    while(1) {
        const struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
        };
        struct addrinfo *res;

        printf("Running DNS lookup for %s...\r\n", WEB_SERVER);
        int err = getaddrinfo(WEB_SERVER, "3000", &hints, &res);

        if(err != 0 || res == NULL) {
            printf("DNS lookup failed err=%d res=%p\r\n", err, res);
            if(res)
                freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_RATE_MS);
            failures++;
            continue;
        }
        /* Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

        int s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            printf("... Failed to allocate socket.\r\n");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_RATE_MS);
            failures++;
            continue;
        }

        printf("... allocated socket\r\n");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            close(s);
            freeaddrinfo(res);
            printf("... socket connect failed.\r\n");
            vTaskDelay(4000 / portTICK_RATE_MS);
            failures++;
            continue;
        }

        printf("... connected\r\n");
        freeaddrinfo(res);
        request[0] = 0;
        snprintf(details, 80, "{\"temp\": %.3f, \"hum\": %.3f, \"mois\": %.3f, \"light\": %.6f}\r\n", temperature, humidity, moisture, light);

        snprintf(request, 300, "POST / HTTP/1.1\r\nHost: %s\r\nUser-Agent: esp-open-rtos/0.1 esp8266\r\nConnection: close\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: %d\r\n\r\n%s\r\n", WEB_URL, strlen(details), details);
        printf("%s", request);
        if (write(s, request, strlen(request)) < 0) {
            printf("... socket send failed\r\n");
            close(s);
            vTaskDelay(4000 / portTICK_RATE_MS);
            failures++;
            continue;
        }
        printf("... socket send success\r\n");

        static char recv_buf[200];
        int r;
        do {
            printf("receiving...");
            bzero(recv_buf, 200);
            r = read(s, recv_buf, 199);
            if(r > 0) {
                printf("%s", recv_buf);
            }
        } while(r > 0);

        printf("... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
        if(r != 0)
            failures++;
        else
            successes++;
        close(s);
        printf("successes = %d failures = %d\r\n", successes, failures);
        vTaskDelay(10000 / portTICK_RATE_MS);
        printf("\r\nStarting again!\r\n");
    }
}
