#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "stdint.h"
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include "FreeRTOS.h"
#include "task.h"
#include <ssid_config.h>
#include "esp8266.h"
#include "../include/dht11.h"

float temperature;
float humidity;
float moisture;

uint8_t dht_data = 14;
uint8_t soil_moisture_vcc = 4;
uint32_t maxcycles = 800000; 

volatile uint32_t frc1_count;
volatile uint32_t frc2_count;
bool valid = true;
uint8_t data[5];

portTickType portTickMs(portTickType ms) {
    return ms / portTICK_RATE_MS;
}

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
