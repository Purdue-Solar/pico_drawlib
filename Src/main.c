#include "ili9341.h"

#ifdef SIMULATION
#include "picosdk_sim.h"
#else
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#endif

#include <stdio.h>
// #include <sys/stdio.h>

int main() {
    stdio_init_all();

    bool led_on = true;

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);

    gpio_init(22);
    gpio_set_dir(22, GPIO_OUT);
    gpio_put(22, led_on);

    LCD_setPins(20, 17, 21, 18, 19);
    LCD_setSPIperiph(spi0);
    LCD_initDisplay();
    LCD_setRotation(1);

#ifdef SIMULATION
    LCDSim_InitWindow();

    while (!LCDSim_WindowShouldClose())
#else
    while (true)
#endif
    {
        printf("EEEEE\n");

        // int q = 0;
        // for (int y = 0; y < 200; y++) {
        //     for (int x = 0; x < 200; x++) {
        //         LCD_WritePixel(x, y, ILI9341_GREEN);
        //         printf("%d\n", q++);
        //     }
        // }

        gpio_put(PICO_DEFAULT_LED_PIN, led_on);
        // gpio_put(22, led_on);
        led_on = !led_on;
        sleep_ms(500);

#ifdef SIMULATION
        LCDSim_Redraw();
#endif
    }

#ifdef SIMULATION
    LCDSim_CloseWindow();
#endif

}
