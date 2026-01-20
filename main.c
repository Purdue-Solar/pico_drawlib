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
    uint32_t send_id = 0x123;
    uint32_t rec_id = 0;
    uint8_t data[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    uint8_t recv_data[8];
    uint8_t recv_len = 0;
    bool led_state = false;
    stdio_init_all(); 
    gpio_init(LED_PIN);
    
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

    gpio_put(LED_PIN, led_state);
    xl2515_init(KBPS125);
    while (true) {
        printf("Hello, world!\n");
        xl2515_send(send_id, data, 8);
        if (xl2515_recv(&rec_id, recv_data, &recv_len))
        {
            printf("recv 0x%x: ", rec_id);
            for (uint8_t i = 0; i < recv_len; i++)
            {
                printf("%02x ", recv_data[i]);
            }
            printf("\r\n");
        }
        led_state = !led_state;
        gpio_put(LED_PIN, led_state);
        sleep_ms(1000);
    }
}
