#include <stdio.h>
#include "pdl.hpp"

extern "C"
{
#include "fonts.h"
#include "gfx.h"
#include "ili9341.h"
#include "icons.h"
}


#define WHITE 0xFFFF

uint16_t bgcolor = ILI9341_CASET;

enum warningSeverity
{
    ERRORLOW,
    WARNINGLOW,
    GOOD,
    WARNINGHIGH,
    ERRORHIGH
};

uint16_t get_color_bat(warningSeverity warning)
{
    uint16_t retval = WHITE;
    switch (warning) {
        case ERRORLOW: retval = ILI9341_MAGENTA;
        case WARNINGLOW: ILI9341_BLUE;
        case GOOD: ILI9341_GREEN;
        case WARNINGHIGH: ILI9341_YELLOW;
        case ERRORHIGH: ILI9341_ORANGE;
    }

    return retval;
}

void pdl_init(spi_inst_t *spi, uint dc, uint cs, uint rst, uint sck, uint mosi)
{
    LCD_setPins(dc, cs, rst, sck, mosi);
    LCD_setSPIperiph(spi);
    LCD_initDisplay();
    LCD_setRotation(1);
    GFX_createFramebuf();
}


// for storing conversions from number to string
char buf[32];
const char *num_to_str(int num)
{
    snprintf(buf, 32, "%d", num);
    return buf;
}

// mix_color(x, y, 0) == x
// mix_color(x, y, 255) == y
static uint16_t mix_color(uint16_t x, uint16_t y, uint8_t a)
{
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

const char *pdl_get_warning_message(const PDLInfo *info)
{
    if ((info -> main_status) & obit(PowerDistroMsg::OutputBit::VoltageHighError)) {return "Voltage High ERROR Main LV Bus";}
    if ((info -> main_status) & obit(PowerDistroMsg::OutputBit::VoltageLowError)) {return "Voltage Low ERROR Main LV Bus";}
    if ((info -> main_status) & obit(PowerDistroMsg::OutputBit::CurrentHighError)) {return "Current High ERROR Main LV Bus";}
    if ((info -> main_status) & obit(PowerDistroMsg::OutputBit::CurrentLowError)) {return "Current Low ERROR Main LV Bus";}

    if ((info -> monitor_status) & mbit(PowerDistroMsg::MonitorBit::DcdcInvalid)) {return "Voltage Invalid ERROR Main LV Bus";}
    if ((info -> monitor_status) & mbit(PowerDistroMsg::MonitorBit::MainMonitorError)) {return "Communication ERROR Main LV Bus";}


    if ((info -> aux_status) & obit(PowerDistroMsg::OutputBit::VoltageHighError)) {return "Voltage High ERROR Aux LV Bus";}
    if ((info -> aux_status) & obit(PowerDistroMsg::OutputBit::VoltageLowError)) {return "Voltage Low ERROR Aux LV Bus";}
    if ((info -> aux_status) & obit(PowerDistroMsg::OutputBit::CurrentHighError)) {return "Current High ERROR Aux LV Bus";}
    if ((info -> aux_status) & obit(PowerDistroMsg::OutputBit::CurrentLowError)) {return "Current Low ERROR Aux LV Bus";}

    if ((info -> monitor_status) & mbit(PowerDistroMsg::MonitorBit::AuxInvalid)) {return "Voltage Invalid ERROR Aux LV Bus";}
    if ((info -> monitor_status) & mbit(PowerDistroMsg::MonitorBit::AuxMonitorError)) {return "Communication ERROR Aux LV Bus";}

    // Note: We don't display warnings because we want to get the user's attention when there is an error

    return NULL;
}

typedef enum FontSize
{
    FNTBIG,
    FNTSMALL,
} FontSize;
typedef struct FontState
{
    const struct mf_font_s *font;
    uint16_t fgcolor;
} FontState;

const struct mf_font_s *get_font_from_fontsize(FontSize size)
{
    switch (size)
    {
    case FNTBIG:
        return &mf_rlefont_Roboto_Regular40.font;
    case FNTSMALL:
        return &mf_rlefont_Roboto_Regular20.font;
    }
    return &mf_rlefont_Roboto_Regular20.font;
}

static void pixel_cb(int16_t x, int16_t y, uint8_t count, uint8_t alpha, void *_state)
{
    FontState *state = (FontState *) _state;
    GFX_drawFastHLine(x, y, count, mix_color(bgcolor, state->fgcolor, alpha));
}

static uint8_t char_cb(int16_t x0, int16_t y0, mf_char character, void *_state)
{
    FontState *state = (FontState *)_state;
    return mf_render_character(state->font, x0, y0, character, pixel_cb, state);
}

void pdl_draw_text(int16_t x0, int16_t y0, enum mf_align_t align, FontSize size, uint16_t color, const char *text)
{
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
static void pdl_draw_stat(const char *name, uint16_t color, int16_t x, int16_t y, enum mf_align_t align)
{
    // `value`'s square's width and height
    const int valw = 15;
    const int valh = 30;
    // width between value square and text
    const int textpad = 5;

    int valx = (align == MF_ALIGN_LEFT) ? x : x - valw;
    GFX_fillRect(valx, y, valw, valh, color);

    int textx = (align == MF_ALIGN_LEFT) ? x + valw + textpad : x - valw - textpad;
    pdl_draw_text(textx, y, align, FNTSMALL, WHITE, name);
}

/**
This function draws fault information for the main and auxillary batteries. Their color of the icon
corresponds to the severity of the warning, which is defined in warningSeverity
 */
static void pdl_draw_battery_stats(
    enum mf_align_t align, int16_t x, int16_t y, uint8_t data)
{
    warningSeverity voltage;
    warningSeverity current;
    using PU = PowerDistroMsg::OutputBit;
    if (obit(PU::VoltageHighError) & data) {voltage = ERRORHIGH;}
    else if (obit(PU::VoltageLowError) & data) {voltage = ERRORLOW;}
    else if (obit(PU::VoltageHighWarn) & data) {voltage = WARNINGHIGH;}
    else if (obit(PU::VoltageLowWarn) & data) {voltage = WARNINGLOW;}
    else {voltage = GOOD;};

    if (obit(PU::CurrentHighError) & data) {current = ERRORHIGH;}
    else if (obit(PU::CurrentLowError) & data) {current = ERRORLOW;}
    else if (obit(PU::CurrentHighWarn) & data) {current = WARNINGHIGH;}
    else if (obit(PU::CurrentLowWarn) & data) {current = WARNINGLOW;}
    else {current = GOOD;};

    pdl_draw_stat("Vt", get_color_bat(voltage), x, y, align);
    y += 40;
    pdl_draw_stat("Cu", get_color_bat(current), x, y, align);
}

/**
This function draws fault information for the power monitors on the power distor board. IF there is a fault,
the color chosen is orange. Either way, the stats are drawn using pdl_draw_stat. 
 */
static void pdl_draw_monitor_stats(
    enum mf_align_t align1, enum mf_align_t align2, int16_t x1, int16_t x2, int16_t y, uint8_t data)
{
    using PM = PowerDistroMsg::MonitorBit;
    uint16_t dcdcFault = (data & mbit(PM::DcdcInvalid)) ? ILI9341_ORANGE : ILI9341_GREEN;
    uint16_t auxFault = (data & mbit(PM::AuxInvalid)) ? ILI9341_ORANGE : ILI9341_GREEN;
    uint16_t commsFault = (data & (mbit(PM::MainMonitorError) | mbit(PM::AuxMonitorError))) ? ILI9341_ORANGE : ILI9341_GREEN;  
    uint16_t debugFault = ILI9341_GREEN; // feel free to use this for debugging!

    pdl_draw_stat("LF", dcdcFault, x1, y, align1);
    pdl_draw_stat("AF", auxFault, x2, y, align2);
    y += 40;
    pdl_draw_stat("CF", commsFault, x1, y, align1);
    pdl_draw_stat("??", debugFault, x2, y, align2);
}

void pdl_draw(const PDLInfo *info)
{
    GFX_setClearColor(bgcolor);
    GFX_clearScreen();
    // GFX_fillRect(0, 0, PDL_WIDTH, PDL_HEIGHT, bgcolor);

    GFX_drawFastVLine(PDL_CENTERPANEL_LEFT, 0, PDL_MAIN_BOTTOM, 0xFFFF);
    GFX_drawFastVLine(PDL_CENTERPANEL_RIGHT, 0, PDL_MAIN_BOTTOM, 0xFFFF);
    GFX_drawFastHLine(0, PDL_MAIN_BOTTOM, PDL_WIDTH, 0xFFFF);

    pdl_draw_text(0, 0, MF_ALIGN_LEFT, FNTSMALL, WHITE, "MAIN");
    pdl_draw_text(PDL_WIDTH, 0, MF_ALIGN_RIGHT, FNTSMALL, WHITE, "AUX");

    pdl_draw_text(PDL_WIDTH / 2, -10, MF_ALIGN_CENTER, FNTBIG, WHITE, num_to_str(info->motor_velocity));
    pdl_draw_text(PDL_WIDTH / 2, 30, MF_ALIGN_CENTER, FNTSMALL, WHITE, "mph");
    pdl_draw_text(PDL_WIDTH / 2, 50, MF_ALIGN_CENTER, FNTBIG, WHITE, num_to_str(info->motor_current));
    pdl_draw_text(PDL_WIDTH / 2, 90, MF_ALIGN_CENTER, FNTSMALL, WHITE, "Amps");

    pdl_draw_battery_stats(
        MF_ALIGN_LEFT, 10, 30, info->main_status);
    pdl_draw_battery_stats(
    MF_ALIGN_RIGHT, PDL_WIDTH - 10, 30, info->aux_status);
    pdl_draw_monitor_stats(MF_ALIGN_LEFT, 
    MF_ALIGN_RIGHT, 10, PDL_WIDTH - 10, 110, info->monitor_status);

    const char *warning = pdl_get_warning_message(info);
    if (warning != NULL)
    {
        GFX_DrawIcon(
            warning_icon,
            5, PDL_HEIGHT - warning_icon_height - 3,
            warning_icon_width, warning_icon_height, ILI9341_ORANGE);
        pdl_draw_text(40, PDL_MAIN_BOTTOM + 5, MF_ALIGN_LEFT, FNTSMALL, ILI9341_YELLOW, warning);
    }

    GFX_flush();
}