/* The "blink" example, but writing the LED from timer interrupts
 *
 * This sample code is in the public domain.
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <ssid_config.h>
#include "esp8266.h"

#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ssid_config.h"

#define WEB_SERVER "192.168.1.132"
#define WEB_PORT 3000
#define WEB_URL "http://192.168.1.132:3000/"
char request[300];
char details[80];
char headers[200];
float temperature;
float humidity;
float moisture;
#define PUB_MSG_LEN 48

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
        request[0] = "\0";
        snprintf(details, 80, "{\"temp\": %.3f, \"hum\": %.3f, \"mois\": %.3f}\r\n", temperature, humidity, moisture);

        snprintf(request, 300, "POST / HTTP/1.1\r\nHost: %s\r\nUser-Agent: esp-open-rtos/0.1 esp8266\r\nConnection: close\r\nContent-Type: application/json; charset=UTF-8\r\nContent-Length: %d\r\n\r\n%s\r\n", WEB_URL, strlen(details), details);
        printf(request);
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

static uint8_t dht_data = 14;
static uint8_t soil_moisture_vcc = 4;

static int maxcycles = 800000; 

static volatile uint32_t frc1_count;
static volatile uint32_t frc2_count;
bool valid = true;
uint8_t data[5];

/* Return a number of ticks to delay based on a time specified 
 * in ms.
 */
portTickType portTickMs(portTickType ms) {
    return ms / portTICK_RATE_MS;
}

/* Return a number of ticks to delay based on a time specified 
 * in us.
 */
portTickType portTickUs(portTickType us) {
    return us / portTICK_RATE_MS / 1000;
}

float convertCtoF(float c) {
  return c * 1.8 + 32;
}

float convertFtoC(float f) {
  return (f - 32) * 0.55555;
}

/* Returns count of loops until pin moves away from level.
 * 
 * The low pulses should be 50us, and the high pulses should be 
 * 28us for 0 bits and 70us for 1 bits. Without knowing the actual
 * timing, we can tell which high pulses are which by checking if
 * they're greater or less than the low pulses.
 */
uint32_t expectPulse(bool level, uint8_t pin) {
    uint32_t count = 0;
    while (gpio_read(pin) == level) {
        if (count++ >= maxcycles) {
            return 0;
        }
    }
    return count;
}

void readTemperature(bool S) {
    float f = -1.0;

    if (valid) {
        f = data[2];
        if(S) {
            f = convertCtoF(f);
        }
        temperature = f;
    }
}

void readHumidity() {
  float f = -1.0;
  if (valid) {
      f = data[0];
      humidity = f;
    }
}

void tempReadTask(void* pvParameters)
{
    // wait 2s for dht11 to settle
    vTaskDelay(portTickMs(2000));
    gpio_enable(soil_moisture_vcc, GPIO_OUTPUT);
    while(1) {
        printf("temp loop\r\n");
        valid = true;
        gpio_enable(dht_data, GPIO_OUTPUT);
        data[0] = data[1] = data[2] = data[3] = data[4] = 0;
        gpio_write(dht_data, 0);
        vTaskDelay(portTickMs(25));
        uint32_t cycles[80];
        gpio_write(dht_data, 1);
        vTaskDelay(portTickUs(40));
        gpio_enable(dht_data, GPIO_INPUT);
        vTaskDelay(portTickUs(10));
        if (expectPulse(false, dht_data) == 0) {
            printf("Timeout waiting for low pulse\r\n");
            valid = false;
        }
        if (expectPulse(true, dht_data) == 0) {
            printf("Timeout waiting for high pulse\r\n");
            valid = false;
        }
        for (int i=0; i<80; i+=2) {
            cycles[i] = expectPulse(false, dht_data);
            cycles[i+1] = expectPulse(true, dht_data);
        }
        // Inspect pulses and determine which ones are 0 (high state cycle count < low
        // state cycle count), or 1 (high state cycle count > low state cycle count).
        for (int i=0; i<40; ++i) {
            uint32_t lowCycles  = cycles[2*i];
            uint32_t highCycles = cycles[2*i+1];
            if ((lowCycles == 0) || (highCycles == 0)) {
                printf("Timeout waiting for pulse.");
                valid = false;
            }
            data[i/8] <<= 1;
            // Now compare the low and high cycle times to see if the bit is a 0 or 1.
            if (highCycles > lowCycles) {
                // High cycles are greater than 50us low cycle count, must be a 1.
                data[i/8] |= 1;
            }
            // Else high cycles are less than (or equal to, a weird case) the 50us low
            // cycle count so this must be a zero.  Nothing needs to be changed in the
            // stored data.
        }
        if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
            valid = false;
        }
        readHumidity();
        readTemperature(true);
        printf("\r\nTemperature is: %f \r\n", temperature);
        printf("Humidity is: %f \r\n", humidity);
        gpio_write(soil_moisture_vcc, 1);
        vTaskDelay(portTickMs(10));
        moisture =  (1.0 / 1024) * sdk_system_adc_read();
        printf ("moisture is %.3f \r\n", moisture); 
        gpio_write(soil_moisture_vcc, 0);
        vTaskDelay(portTickMs(15000));
    }
}

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
