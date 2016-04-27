#include "FreeRTOS.h"
#include "task.h"
#include "espressif/esp_common.h"

extern float temperature;
extern float humidity;
extern float moisture;

extern uint8_t dht_data;
extern uint8_t soil_moisture_vcc;
extern uint32_t maxcycles; 

extern volatile uint32_t frc1_count;
extern volatile uint32_t frc2_count;
extern bool valid;
extern uint8_t data[5];

/* Return a number of ticks to delay based on a time specified 
 * in ms.
 */
portTickType portTickMs(portTickType ms);

/* Return a number of ticks to delay based on a time specified 
 * in us.
 */
portTickType portTickUs(portTickType us);

float convertCtoF(float c);
float convertFtoC(float f);

/* Returns count of loops until pin moves away from level.
 * 
 * The low pulses should be 50us, and the high pulses should be 
 * 28us for 0 bits and 70us for 1 bits. Without knowing the actual
 * timing, we can tell which high pulses are which by checking if
 * they're greater or less than the low pulses.
 */
uint32_t expectPulse(bool level, uint8_t pin);
void readTemperature(bool S);
void readHumidity();
void tempReadTask(void* pvParameters);
