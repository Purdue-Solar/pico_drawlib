#ifdef SIMULATION

// How big the window should be
#define WINSCALE 2

#include <memory.h>
#include "ili9341.h"
#include <raylib.h>
#include <stdio.h>

uint16_t _width;  ///< Display width as modified by current rotation
uint16_t _height; ///< Display height as modified by current rotation

uint8_t rotation;

uint16_t buffer[ILI9341_TFTWIDTH * ILI9341_TFTHEIGHT];
Texture2D texture;

void LCD_setPins(uint16_t dc, uint16_t cs, int16_t rst, uint16_t sck, uint16_t tx) {

}
void LCD_setSPIperiph(spi_inst_t *s) {

}
void LCD_initDisplay() {
    _width  = ILI9341_TFTWIDTH;
    _height = ILI9341_TFTHEIGHT;
}

void LCDSim_InitWindow() {
    InitWindow(_width * WINSCALE, _height * WINSCALE, "ILI9341 Simulation");
    texture = LoadTextureFromImage((Image) {
        .data = NULL,
        .width = _width,
        .height = _height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R5G6B5,
    });
}

int LCDSim_WindowShouldClose(void) {
    return WindowShouldClose();
}
void LCDSim_CloseWindow(void) {
    CloseWindow();
}

void LCD_setRotation(uint8_t m) {
    rotation = m % 4;

    switch (rotation) {
    case 0:
    case 2:
        _width  = ILI9341_TFTWIDTH;
        _height = ILI9341_TFTHEIGHT;
        break;

    case 1:
    case 3:
        _width  = ILI9341_TFTHEIGHT;
        _height = ILI9341_TFTWIDTH;
        break;
    }
}

void LCDSim_Redraw() {
    // puts("redrawing.");
    UpdateTexture(texture, buffer);
    BeginDrawing();
    ClearBackground(BLUE);
    // DrawTexture(texture, 0, 0, WHITE);
    DrawTextureEx(texture, (Vector2){0.0, 0.0}, 0.0, WINSCALE, WHITE);
    EndDrawing();
}

void LCD_WritePixel(int x, int y, uint16_t col) {
    buffer[y * _width + x] = col;
    // LCDSim_Redraw();
}

void LCD_WriteBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    for (uint16_t myy = y; myy < (y + h); myy++) {
        memcpy(&buffer[myy * _width + x], &bitmap[(myy - y) * w], sizeof(uint16_t) * w);
    }
    // LCDSim_Redraw();
}

#endif
