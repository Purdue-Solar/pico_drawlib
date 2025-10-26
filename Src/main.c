#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "pico/stdlib.h"
#include "display.h"
#include "mcufont.h"
#include "mf_font.h"
#include "mf_justify.h"
#include "fonts.h"
#include "lcd_st7789_library.h"

#define LCD_WIDTH 240
#define LCD_HEIGHT 320

// uint16_t row_pixels[LCD_WIDTH];
uint16_t fbuf[LCD_WIDTH * LCD_HEIGHT];
uint16_t bbuf[LCD_WIDTH * LCD_HEIGHT];

static void render_character_pixels(int16_t x, int16_t y, uint8_t count, uint8_t alpha, void *state)
{
    uint16_t *buf = state;
    while (count--)
    {
        fbuf[(LCD_HEIGHT - (x++)) * LCD_WIDTH + y] = RGB(alpha >> 3, alpha >> 2, alpha >> 3);
    }
}

static uint8_t render_character(int16_t x0, int16_t y0, mf_char character, void *state)
{
    return mf_render_character(&mf_bwfont_Roboto_Regular20bw.font, x0, y0, character, render_character_pixels, state);
}

int main()
{
    stdio_init_all();
    lcd_init();
    lcd_fill_color(COLOR_CYAN);
    lcd_draw_rect(10, 10, 100, 50, COLOR_RED);
    lcd_draw_text(20, 20, "Hello, World!", COLOR_WHITE, COLOR_WHITE, 5);
    lcd_draw_filled_circle(100, 100, 30, COLOR_GREEN);

    while (1)
    {
        tight_loop_contents();
    }

    return 0;
}