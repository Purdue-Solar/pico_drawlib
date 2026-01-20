#include <stdio.h>
#include "pico/stdlib.h"
#include "xl2515.h"

#define LED_PIN 25
#define R0 16
#define R1 17
#define R2 13
#define C0 15
#define C1 14
#define C2 18
#define C3 19

int main()
{
    stdio_init_all(); 
    
    gpio_init(R0);
    gpio_init(R1);
    gpio_init(R2);
    gpio_init(C0);
    gpio_init(C1);
    gpio_init(C2);
    gpio_init(C3);
    gpio_set_dir(R0, GPIO_IN);
    gpio_set_dir(R1, GPIO_IN);
    gpio_set_dir(R2, GPIO_IN);
    gpio_set_dir(C0, GPIO_OUT);
    gpio_set_dir(C1, GPIO_OUT);
    gpio_set_dir(C2, GPIO_OUT);
    gpio_set_dir(C3, GPIO_OUT);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        for (int i = 0; i < 4; i++) {
            gpio_put(C0, i==0);
            gpio_put(C1, i==1);
            gpio_put(C2, i==2);
            gpio_put(C3, i==3);

            if (gpio_get(R0) && i==3)
            {
                // Brights
            }
            sleep_ms(10);

        }
    }
}
