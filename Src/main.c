#include "ili9341.h"
#include "gfx.h"

#include "fonts.h"
#include "mcufont.h"
#include "icons.h"

// #include "psrcar.h"

#ifdef SIMULATION
#include "picosdk_sim.h"
#else
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#endif

#include <stdio.h>
// #include <sys/stdio.h>

static void pixel_cb(int16_t x, int16_t y, uint8_t count, uint8_t alpha, void* state) {
    GFX_drawFastHLine(x, y, count, GFX_RGB565(alpha, alpha, alpha));
}

static uint8_t char_cb(int16_t x0, int16_t y0, mf_char character, void* state) {
    return mf_render_character(&mf_bwfont_Roboto_Regular20bw.font, x0, y0, character, pixel_cb, state);
}

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

    GFX_createFramebuf();

#ifdef SIMULATION
    LCDSim_InitWindow();

    while (!LCDSim_WindowShouldClose())
#else
    while (true)
#endif
    {
        GFX_setClearColor(ILI9341_CASET);
        GFX_clearScreen();

        GFX_setCursor(0, 0);
        GFX_setTextColor(ILI9341_GREEN);
        GFX_setTextBack(ILI9341_GREEN);
        GFX_printf("I am the built-in font from the ILI9341 Library!");

        mf_render_aligned(&mf_bwfont_Roboto_Regular20bw.font, GFX_getWidth() / 2, GFX_getHeight() / 2, MF_ALIGN_CENTER, "I am MCUFont.", 0, char_cb, NULL);

        GFX_setTextColor(ILI9341_WHITE);
        GFX_DrawIcon(warning_icon, 0, 100, 40, 40);

        GFX_Update();
#ifdef SIMULATION
        LCDSim_Redraw();
#endif
    }

    GFX_destroyFramebuf();

#ifdef SIMULATION
    LCDSim_CloseWindow();
#endif
}
