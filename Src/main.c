/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "mf_font.h"
#include "mf_justify.h"
#include "pico/stdlib.h"
#include "st7789.h"
#include "fonts.h"
#include "mcufont.h"

#define RGB(r, g, b) (((r) << 11) | ((g) << 5) | (b))
#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// lcd configuration
const struct st7789_config lcd_config = {
    .spi      = spi0,
    .gpio_din = PICO_DEFAULT_SPI_TX_PIN,
    .gpio_clk = PICO_DEFAULT_SPI_SCK_PIN,
    .gpio_cs  = PICO_DEFAULT_SPI_CSN_PIN,
    .gpio_dc  = 20,
    .gpio_rst = 21,
    .gpio_bl  = 22,
};

// uint16_t row_pixels[LCD_WIDTH];
uint16_t fbuf[LCD_WIDTH * LCD_HEIGHT];
uint16_t bbuf[LCD_WIDTH * LCD_HEIGHT];

static void pixel_cb(int16_t x, int16_t y, uint8_t count, uint8_t alpha, void* state) {
    uint16_t* buf = state;
    while (count--) {
        fbuf[(LCD_HEIGHT - (x++)) * LCD_WIDTH + y] = RGB(alpha >> 3, alpha >> 2, alpha >> 3);
    }
}

static uint8_t char_cb(int16_t x0, int16_t y0, mf_char character, void* state) {
    return mf_render_character(&mf_bwfont_Roboto_Regular20bw.font, x0, y0, character, pixel_cb, state);
}

int main()
{
    memset(fbuf, 0x00, 2 * LCD_WIDTH * LCD_HEIGHT);
    memset(bbuf, 0xFF, 2 * LCD_WIDTH * LCD_HEIGHT);

    mf_render_aligned(&mf_bwfont_Roboto_Regular20bw.font, LCD_HEIGHT / 2, LCD_WIDTH / 2,
        MF_ALIGN_CENTER, "Purdue Solar", 0, char_cb, fbuf);

    // initialize the lcd
    st7789_init(&lcd_config, LCD_WIDTH, LCD_HEIGHT);

    while (1) {
        // // make screen black
        // st7789_fill(0x0000);

        // // wait 1 second
        // sleep_ms(1000);

        // // make screen white
        // st7789_fill(0xffff);

        // // wait 1 second
        // sleep_ms(1000);

        st7789_write(fbuf, 2 * LCD_WIDTH * LCD_HEIGHT);
        sleep_ms(1000);
        st7789_write(bbuf, 2 * LCD_WIDTH * LCD_HEIGHT);
        sleep_ms(1000);

        // st7789_fill(RGB(31, 63, 31));
        // sleep_ms(1000);
        // st7789_fill(RGB(31, 0, 0));
        // sleep_ms(1000);
        // st7789_fill(RGB(0, 63, 0));
        // sleep_ms(1000);
        // st7789_fill(RGB(0, 0, 31));
        // sleep_ms(1000);
        // st7789_fill(RGB(31, 63, 0));
        // sleep_ms(1000);
        // st7789_fill(RGB(0, 63, 31));
        // sleep_ms(1000);
        // st7789_fill(RGB(31, 0, 31));
        // sleep_ms(1000);
        // st7789_fill(RGB(0, 0, 0));
        // sleep_ms(1000);
    }

    return 0;
}