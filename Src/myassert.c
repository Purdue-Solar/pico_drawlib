#include "myassert.h"

void myassert(bool val)
{
    if(val)
    {
        return;
    }

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    for(;;)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(50);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(500);
    }
}

void myassert_checkpoint(bool val, int n_blink)
{
    if(val)
    {
        return;
    }

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    for(;;)
    {
        for(int i = 0; i < n_blink; ++i)
        {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            sleep_ms(50);
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            sleep_ms(50);
        }
        sleep_ms(1000);
    }
}
