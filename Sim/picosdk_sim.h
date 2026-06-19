#ifndef PICOSDK_SIM_H
#define PICOSDK_SIM_H

#define PICO_DEFAULT_LED_PIN 25
#define GPIO_OUT 1u

#include <stdbool.h>
#include <stdint.h>
#ifndef _WIN32
#include <unistd.h>
#endif

typedef unsigned int uint;

void gpio_init(uint gpio);
void gpio_set_dir(uint gpio, bool out);
void gpio_put(uint gpio, bool value);
void sleep_ms(uint32_t ms);
bool stdio_init_all(void);

typedef void spi_inst_t;
#define spi0 ((spi_inst_t*)(NULL))

#endif // !PICOSDK_SIM_H
