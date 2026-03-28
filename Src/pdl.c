#include <stdio.h>
#include "pdl.h"
#include "fonts.h"
#include "gfx.h"
#include "ili9341.h"
#include "icons.h"

#define WHITE 0xFFFF

uint16_t bgcolor = ILI9341_CASET;

// for storing conversions from number to string
char buf[32];
const char* num_to_str(int num) {
    snprintf(buf, 32, "%d", num);
    return buf;
}

// mix_color(x, y, 0) == x
// mix_color(x, y, 255) == y
static uint16_t mix_color(uint16_t x, uint16_t y, uint8_t a) {
    uint16_t xr = x >> 11;
    uint16_t yr = y >> 11;

    uint16_t xg = (x >> 5) & 0b111111;
    uint16_t yg = (y >> 5) & 0b111111;

    uint16_t xb = x & 0b11111;
    uint16_t yb = y & 0b11111;

    uint8_t na = 255 - a;

    uint16_t r = ((xr * na) + (yr * a)) >> 8;
    uint16_t g = ((xg * na) + (yg * a)) >> 8;
    uint32_t b = ((xb * na) + (yb * a)) >> 8;

    return (r << 11) | (g << 5) | b;
}

const char* pdl_get_warning_message(const PDLInfo* info) {
    if (info->main_current_warning) {
        return "Main battery current warning";
    }
    if (info->aux_current_warning) {
        return "Aux battery current warning";
    }
    if (info->main_over_current) {
        return "Main battery overcurrent";
    }
    if (info->aux_over_current) {
        return "Aux battery overcurrent";
    }
    if (info->main_over_voltage) {
        return "Main battery overvoltage";
    }
    if (info->aux_over_voltage) {
        return "Aux battery overvoltage";
    }
    if (info->main_under_voltage) {
        return "Main battery undervoltage";
    }
    if (info->aux_under_voltage) {
        return "Aux battery undervoltage";
    }
    // if (info->aux_condition) {
    //     return "Aux battery condition";
    // }
    if (!info->main_valid) {
        return "Main battery invalid";
    }
    if (!info->aux_valid) {
        return "Aux battery invalid";
    }

    return NULL;
}

typedef enum FontSize {
    FNTBIG,
    FNTSMALL,
} FontSize;
typedef struct FontState {
    const struct mf_font_s* font;
    uint16_t fgcolor;
} FontState;

const struct mf_font_s* get_font_from_fontsize(FontSize size) {
    switch (size) {
        case FNTBIG:   return &mf_rlefont_Roboto_Regular40.font;
        case FNTSMALL: return &mf_rlefont_Roboto_Regular20.font;
    }
}

static void pixel_cb(int16_t x, int16_t y, uint8_t count, uint8_t alpha, void* _state) {
    FontState* state = _state;
    GFX_drawFastHLine(x, y, count, mix_color(bgcolor, state->fgcolor, alpha));
}

static uint8_t char_cb(int16_t x0, int16_t y0, mf_char character, void* _state) {
    FontState* state = _state;
    return mf_render_character(state->font, x0, y0, character, pixel_cb, state);
}

void pdl_draw_text(int16_t x0, int16_t y0, enum mf_align_t align, FontSize size, uint16_t color, const char* text) {
    FontState state = {
        .font = get_font_from_fontsize(size),
        .fgcolor = color,
    };
    mf_render_aligned(state.font, x0, y0, align, text, 0, char_cb, &state);
}

// align is either left or right
// if align is left,  then things are drawn to the RIGHT of (x, y)
// if align is right, then things are drawn to the LEFT  of (x, y)
// I know this is confusing, but this is EXACTLY how MCUFont does it.
// I'm sorry.
// (also I know we're using an MCUFont enum here, but I don't want to declare a new enum)
static void pdl_draw_stat(const char* name, bool value, int16_t x, int16_t y, enum mf_align_t align) {
    // `value`'s square's width and height
    const int valw = 15;
    const int valh = 30;
    // width between value square and text
    const int textpad = 5;
    const uint16_t truecolor  = ILI9341_GREEN;
    const uint16_t falsecolor = ILI9341_RED;

    int valx = (align == MF_ALIGN_LEFT) ? x : x - valw;
    GFX_fillRect(valx, y, valw, valh, value ? truecolor : falsecolor);
    
    int textx = (align == MF_ALIGN_LEFT) ? x + valw + textpad : x - valw - textpad;
    pdl_draw_text(textx, y, align, FNTSMALL, WHITE, name);
}

static void pdl_draw_battery_stats(
    enum mf_align_t align, int16_t x, int16_t y,
    bool overvoltage, bool undervoltage, bool overcurrent, bool current_warning
) {
    // "false" in pdl_draw_state means "BAD", so we NOT the booleans.
    pdl_draw_stat("OV", !overvoltage,     x, y, align);
    y += 40;
    pdl_draw_stat("UV", !undervoltage,    x, y, align);
    y += 40;
    pdl_draw_stat("OC", !overcurrent,     x, y, align);
    y += 40;
    pdl_draw_stat("CW", !current_warning, x, y, align);
}

void pdl_draw(const PDLInfo* info) {
    GFX_setClearColor(bgcolor);
    GFX_clearScreen();

    GFX_drawFastVLine(PDL_CENTERPANEL_LEFT,  0, PDL_MAIN_BOTTOM, 0xFFFF);
    GFX_drawFastVLine(PDL_CENTERPANEL_RIGHT, 0, PDL_MAIN_BOTTOM, 0xFFFF);
    GFX_drawFastHLine(0, PDL_MAIN_BOTTOM, PDL_WIDTH, 0xFFFF);

    pdl_draw_text(0,         0, MF_ALIGN_LEFT,  FNTSMALL, WHITE, "MAIN");
    pdl_draw_text(PDL_WIDTH, 0, MF_ALIGN_RIGHT, FNTSMALL, WHITE, "AUX");

    pdl_draw_text(PDL_WIDTH / 2, -10, MF_ALIGN_CENTER, FNTBIG,   WHITE, num_to_str(info->motor_velocity));
    pdl_draw_text(PDL_WIDTH / 2, 30,  MF_ALIGN_CENTER, FNTSMALL, WHITE, "mph");
    pdl_draw_text(PDL_WIDTH / 2, 50,  MF_ALIGN_CENTER, FNTBIG,   WHITE, num_to_str(info->motor_current));
    pdl_draw_text(PDL_WIDTH / 2, 90,  MF_ALIGN_CENTER, FNTSMALL, WHITE, "Amps");

    pdl_draw_battery_stats(
        MF_ALIGN_LEFT, 10, 30,
        info->main_over_voltage, info->main_under_voltage,
        info->main_over_current, info->main_current_warning
    );
    pdl_draw_battery_stats(
        MF_ALIGN_RIGHT, PDL_WIDTH - 10, 30,
        info->aux_over_voltage, info->aux_under_voltage,
        info->aux_over_current, info->aux_current_warning
    );

    const char* warning = pdl_get_warning_message(info);
    if (warning != NULL) {
        GFX_DrawIcon(
            warning_icon,
            5, PDL_HEIGHT - warning_icon_height - 3,
            warning_icon_width, warning_icon_height, ILI9341_ORANGE
        );
        pdl_draw_text(40, PDL_MAIN_BOTTOM + 5, MF_ALIGN_LEFT, FNTSMALL, ILI9341_YELLOW, warning);
    }
}
