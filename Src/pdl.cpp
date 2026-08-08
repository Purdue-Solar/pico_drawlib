#include <stdio.h>
#include <type_traits>
#include "pdl.hpp"
#include "artemis_canid.hpp"

extern "C"
{
#include "fonts.h"
#include "gfx.h"
#include "ili9341.h"
#include "icons.h"
}

#define WHITE 0xFFFF
uint16_t bgcolor = ILI9341_CASET; // Note: you usually use colors like ILI9341_BLUE for this, but ILI9341_CASET just happened to create the perfect color

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
        case ERRORLOW:    retval = ILI9341_MAGENTA; break;
        case WARNINGLOW:  retval = ILI9341_BLUE;    break;
        case GOOD:        retval = ILI9341_GREEN;   break;
        case WARNINGHIGH: retval = ILI9341_YELLOW;  break;
        case ERRORHIGH:   retval = ILI9341_ORANGE;  break;
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

// ─── Warning banner (highest-priority active condition) ───────────────────────
// Priority order: isolation → CAN stale → battery faults →
// motor faults → LV faults → LV warnings.
// Note that this is distinct from the highest priority
// items with a red/yellow banner

const char *pdl_get_warning_message(const PDLInfo *info)
{
    using D1  = BmsDtcFlags1;
    using D21 = BmsDtcFlags21;
    using D22 = BmsDtcFlags22;
    using DM  = DistroDisplayMain;
    using DA  = DistroDisplayAux;
    using DMi = DistroDisplayMisc;
    using ML  = McLimitFlags;

    // Isolation fault
    if (info->bms_dtc_flags2_2 & sbit(D22::HighVoltageIsolationFault))
        return "ISOLATION FAULT";

    // CAN state — most safety-critical messages first
    if (info->bms_safety_stale)   return "CAN: BMS Safety Stale";
    if (info->bms_voltage_stale)  return "CAN: BMS Voltage Stale";
    if (info->bms_power_stale)    return "CAN: BMS Power Stale";
    if (info->mc_errors_stale)    return "CAN: MC Errors Stale";
    if (info->mc_bus_stale)       return "CAN: MC Bus Stale";
    if (info->mc_speed_stale)     return "CAN: MC Speed Stale";
    if (info->mc_temp_stale)      return "CAN: MC Temp Stale";
    if (info->power_distro_stale) return "CAN: Distro Stale";

    // Battery voltage error (high before low)
    if (info->bms_dtc_flags2_1 & sbit(D21::HighestCellVoltageAbove5vFault))
        return "Battery Voltage HIGH";
    if ((info->bms_dtc_flags1  & (sbit(D1::HighestCellVoltageTooLowFault) | sbit(D1::LowestCellVoltageTooLowFault))) ||
        (info->bms_dtc_flags2_1 & (sbit(D21::LowCellVoltageFault) | sbit(D21::WeakCellFault))))
        return "Battery Voltage LOW";

    // Battery current error
    if ((info->bms_dtc_flags1  & sbit(D1::DischargeLimitEnforcementFault)) ||
        (info->bms_dtc_flags2_1 & sbit(D21::CurrentSensorFault))           ||
        (info->bms_dtc_flags2_2 & sbit(D22::ChargeLimitEnforcementFault)))
        return "Battery Current Fault";

    // Battery temp error
    if ((info->bms_dtc_flags1  & sbit(D1::PackTooHotFault)) ||
        (info->bms_dtc_flags2_2 & (sbit(D22::ThermistorFault) | sbit(D22::FanMonitorFault))))
        return "Battery Temp Fault";

    // Misc BMS error (remaining DTC flags not covered above)
    {
        const uint8_t misc1  = sbit(D1::ChargerSafetyRelayFault)          |
                               sbit(D1::InternalHardwareFault)             |
                               sbit(D1::InternalHeatsinkThermistorFault)   |
                               sbit(D1::InternalSoftwareFault);
        const uint8_t misc21 = sbit(D21::InternalCommunicationFault)      |
                               sbit(D21::CellBalancingStuckOffFault)       |
                               sbit(D21::OpenWiringFault)                  |
                               sbit(D21::CellAsicFault);
        const uint8_t misc22 = sbit(D22::WeakPackFault)                   |
                               sbit(D22::ExternalCommunicationFault)       |
                               sbit(D22::RedundantPowerSupplyFault)        |
                               sbit(D22::InputPowerSupplyFault);
        if ((info->bms_dtc_flags1  & misc1)  ||
            (info->bms_dtc_flags2_1 & misc21) ||
            (info->bms_dtc_flags2_2 & misc22))
            return "BMS Fault";
    }

    // Aux LV fault (hardware or comms)
    if ((info->monitor_status & sbit(DMi::AuxHardwareDetectedFault)) ||
        (info->monitor_status & sbit(DMi::AuxPowerMonitorI2cError)))
        return "Aux LV Fault";

    // Motor fault (error flags bits 16–32)
    if (info->mc_error_flags1 || info->mc_error_flags2)
        return "Motor Controller Fault";

    // Main LV fault (hardware or comms)
    if ((info->monitor_status & sbit(DMi::MainHardwareDetectedFault)) ||
        (info->monitor_status & sbit(DMi::MainPowerMonitorI2cError)))
        return "Main LV Fault";

    // Aux LV over/under voltage and current errors
    if (info->aux_status & sbit(DA::AuxOverVoltageError))    return "Aux LV Overvoltage ERROR";
    if (info->aux_status & sbit(DA::AuxUnderVoltageError))   return "Aux LV Undervoltage ERROR";
    if (info->aux_status & sbit(DA::AuxOverCurrentError))    return "Aux LV Overcurrent ERROR";
    if (info->aux_status & sbit(DA::AuxUnderCurrentError))   return "Aux LV Undercurrent ERROR";
    // Main LV over/under voltage and current errors
    if (info->main_status & sbit(DM::MainOverVoltageError))    return "Main LV Overvoltage ERROR";
    if (info->main_status & sbit(DM::MainUnderVoltageError))   return "Main LV Undervoltage ERROR";
    if (info->main_status & sbit(DM::MainOverCurrentError))    return "Main LV Overcurrent ERROR";
    if (info->main_status & sbit(DM::MainUnderCurrentError))   return "Main LV Undercurrent ERROR";

     // Motor temp warning (IPM/motor temperature limit active)
    if (info->mc_limit_flags & sbit(ML::IpmMotorTemperature))
        return "Motor Temp Limit";

    // Aux LV over/under voltage and current warnings
    if (info->aux_status & sbit(DA::AuxOverVoltageWarning))  return "Aux LV Overvoltage WARN";
    if (info->aux_status & sbit(DA::AuxUnderVoltageWarning)) return "Aux LV Undervoltage WARN";
    if (info->aux_status & sbit(DA::AuxOverCurrentWarning))  return "Aux LV Overcurrent WARN";
    if (info->aux_status & sbit(DA::AuxUnderCurrentWarning)) return "Aux LV Undercurrent WARN";
    
    // Main LV over/under voltage and current warnings
    if (info->main_status & sbit(DM::MainOverVoltageWarning))  return "Main LV Overvoltage WARN";
    if (info->main_status & sbit(DM::MainUnderVoltageWarning)) return "Main LV Undervoltage WARN";
    if (info->main_status & sbit(DM::MainOverCurrentWarning))  return "Main LV Overcurrent WARN";
    if (info->main_status & sbit(DM::MainUnderCurrentWarning)) return "Main LV Undercurrent WARN";

    return NULL;
}

// ─── Font / text rendering ────────────────────────────────────────────────────

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

// ─── LV peripheral status indicators (used on diagnostics page) ──────────────

// align is either left or right
// if align is left,  things are drawn to the RIGHT of (x, y)
// if align is right, things are drawn to the LEFT  of (x, y)
static void pdl_draw_stat(const char *name, uint16_t color, int16_t x, int16_t y, enum mf_align_t align)
{
    const int valw = 15;
    const int valh = 30;
    const int textpad = 5;

    int valx = (align == MF_ALIGN_LEFT) ? x : x - valw;
    GFX_fillRect(valx, y, valw, valh, color);

    int textx = (align == MF_ALIGN_LEFT) ? x + valw + textpad : x - valw - textpad;
    pdl_draw_text(textx, y, align, FNTSMALL, WHITE, name);
}

// ByteEnum must be DistroDisplayMain (paired with info->main_status) or
// DistroDisplayAux (paired with info->aux_status). Both bytes share identical
// bit positions; the template parameter ties each call site to the correct byte.
template<typename ByteEnum>
static void pdl_draw_battery_stats(mf_align_t align, int16_t x, int16_t y, uint8_t data)
{
    static_assert(
        std::is_same_v<ByteEnum, DistroDisplayMain> ||
        std::is_same_v<ByteEnum, DistroDisplayAux>,
        "ByteEnum must be DistroDisplayMain or DistroDisplayAux"
    );

    // DistroDisplayMain and DistroDisplayAux encode V/I severity at identical
    // bit positions. DistroDisplayMain's names are used as the canonical reference.
    using DM = DistroDisplayMain;
    warningSeverity voltage, current;

    if      (data & sbit(DM::MainOverVoltageError))    voltage = ERRORHIGH;
    else if (data & sbit(DM::MainUnderVoltageError))   voltage = ERRORLOW;
    else if (data & sbit(DM::MainOverVoltageWarning))  voltage = WARNINGHIGH;
    else if (data & sbit(DM::MainUnderVoltageWarning)) voltage = WARNINGLOW;
    else                                               voltage = GOOD;

    if      (data & sbit(DM::MainOverCurrentError))    current = ERRORHIGH;
    else if (data & sbit(DM::MainUnderCurrentError))   current = ERRORLOW;
    else if (data & sbit(DM::MainOverCurrentWarning))  current = WARNINGHIGH;
    else if (data & sbit(DM::MainUnderCurrentWarning)) current = WARNINGLOW;
    else                                               current = GOOD;

    pdl_draw_stat("Vt", get_color_bat(voltage), x, y, align);
    y += 40;
    pdl_draw_stat("Cu", get_color_bat(current), x, y, align);
}

static void pdl_draw_monitor_stats(
    mf_align_t align1, mf_align_t align2, int16_t x1, int16_t x2, int16_t y, uint8_t data)
{
    using DMi = DistroDisplayMisc;
    uint16_t mainHwFault = (data & sbit(DMi::MainHardwareDetectedFault)) ? ILI9341_ORANGE : ILI9341_GREEN;
    uint16_t auxHwFault  = (data & sbit(DMi::AuxHardwareDetectedFault))  ? ILI9341_ORANGE : ILI9341_GREEN;
    uint16_t commsFault  = (data & (sbit(DMi::MainPowerMonitorI2cError) |
                                    sbit(DMi::AuxPowerMonitorI2cError))) ? ILI9341_ORANGE : ILI9341_GREEN;
    uint16_t distroBad   = (data & sbit(DMi::DistroBad)) ? ILI9341_ORANGE : ILI9341_GREEN;

    pdl_draw_stat("MF", mainHwFault, x1, y, align1);
    pdl_draw_stat("AF", auxHwFault,  x2, y, align2);
    y += 40;
    pdl_draw_stat("CF", commsFault,  x1, y, align1);
    pdl_draw_stat("DB", distroBad,   x2, y, align2);
}

// ─── Page-specific helpers ────────────────────────────────────────────────────

static const char *pdl_get_vehicle_state(const PDLInfo *info)
{
    if (info->bms_safety_stale || info->power_distro_stale) return "---";

    using RS1 = BmsRelayState1;
    using DMi = DistroDisplayMisc;

    const uint8_t monitor_fault_mask = sbit(DMi::MainHardwareDetectedFault) |
                                       sbit(DMi::AuxHardwareDetectedFault)  |
                                       sbit(DMi::MainPowerMonitorI2cError)  |
                                       sbit(DMi::AuxPowerMonitorI2cError);

    bool discharge_enabled = (info->bms_relay_state1 & sbit(RS1::DischargeRelayEnabled)) != 0;
    bool monitor_fault     = (info->monitor_status & monitor_fault_mask) != 0;
    bool precharge_active  = (info->monitor_status & sbit(DMi::PowerDistroPrecharge)) != 0;

    if (monitor_fault || !discharge_enabled)
        return "Fault";

    if (precharge_active)
        return "Precharge";

    return "Running";
}

static const char *pdl_get_current_state(const PDLInfo *info)
{
    if (info->mc_errors_stale) return "---";

    using MC = McLimitFlags;

    if (info -> mc_limit_flags & sbit(MC::IpmMotorTemperature)) {
        return "Temp";
    } else if (info -> mc_limit_flags & sbit(MC::BusVoltageUpperLimit)) {
        return "VHigh";
    } else if (info -> mc_limit_flags & sbit(MC::BusVoltageLowerLimit)) {
        return "VLow";
    } else if (info -> mc_limit_flags & sbit(MC::BusCurrent)) {
        return "BusI";
    } else if (info -> mc_limit_flags & sbit(MC::Velocity)) {
        return "Velocity";
    } else if (info -> mc_limit_flags & sbit(MC::OutputVoltagePwm)) {
        return "PWM";
    } 
    return "All good";
}

static int pdl_count_warnings(const PDLInfo *info)
{
    return __builtin_popcount(info->bms_dtc_flags1)
         + __builtin_popcount(info->bms_dtc_flags2_1)
         + __builtin_popcount(info->bms_dtc_flags2_2)
         + __builtin_popcount(info->mc_error_flags1)
         + __builtin_popcount(info->mc_error_flags2)
         + (info->bms_safety_stale == true)
         + (info->bms_voltage_stale == true) 
         + (info->bms_power_stale == true) 
         + (info->mc_errors_stale == true) 
         + (info->mc_bus_stale == true) 
         + (info->mc_speed_stale == true) 
         + (info->mc_temp_stale == true)   
         + (info->power_distro_stale == true);
}

// ─── Main page ────────────────────────────────────────────────────────────────
//
// Layout (320×200 content area, y=0..200):
//   y=2    Vehicle speed (FNTBIG, center)
//   y=44   "mph" label (FNTSMALL, center)
//   y=68   SOC  |  Pack Power
//   y=94   DCL  |  Current limit status
//   y=120  Vehicle state  |  Heatsink temp
//   y=152  Warning/Error count (center, yellow if >0)

static void pdl_draw_main_page(const PDLInfo *info)
{
    char line[48];

    pdl_draw_text(PDL_WIDTH / 2, 2,  MF_ALIGN_CENTER, FNTBIG,   WHITE, num_to_str((int)info->vehicle_velocity));
    pdl_draw_text(PDL_WIDTH / 2, 44, MF_ALIGN_CENTER, FNTSMALL, WHITE, "mph");

    const int16_t L = 5;
    const int16_t R = PDL_WIDTH - 5;

    // Bus Current | Heatsink temp
    snprintf(line, sizeof(line), "BusI: %.1fA", info->motor_bus_current);
    pdl_draw_text(L, 68, MF_ALIGN_LEFT, FNTSMALL, WHITE, line);
    snprintf(line, sizeof(line), "HSTemp: %.0fC", info->heat_sink_temp);
    pdl_draw_text(R, 68, MF_ALIGN_RIGHT, FNTSMALL, WHITE, line);

    // DCL | SOC
    snprintf(line, sizeof(line), "MaxBusI: %dA", info->pack_dcl);
    pdl_draw_text(L, 94, MF_ALIGN_LEFT,  FNTSMALL, WHITE, line);
    snprintf(line, sizeof(line), "SOC: %d%%", info->battery_soc);
    pdl_draw_text(R, 94, MF_ALIGN_RIGHT,  FNTSMALL, WHITE, line);

    // Current limit status | Vehicle state
    snprintf(line, sizeof(line), "ILimit: %s", pdl_get_current_state(info));
    pdl_draw_text(L, 120, MF_ALIGN_LEFT, FNTSMALL, WHITE, line);
    snprintf(line, sizeof(line), "Mode: %s", pdl_get_vehicle_state(info));
    pdl_draw_text(R, 120, MF_ALIGN_RIGHT,  FNTSMALL, WHITE, line);

    // Warning/Error count
    int wcount = pdl_count_warnings(info);
    uint16_t wcolor = (wcount > 0) ? ILI9341_YELLOW : WHITE;
    snprintf(line, sizeof(line), "Warnings/Errors: %d", wcount);
    pdl_draw_text(PDL_WIDTH / 2, 152, MF_ALIGN_CENTER, FNTSMALL, wcolor, line);

    // Power hold indicator — shown when active, centered near the bottom of the content area
    if (info->power_hold_enabled) {
        snprintf(line, sizeof(line), "Power Hold: %dA", info->power_hold_amps);
        pdl_draw_text(PDL_WIDTH / 2, 174, MF_ALIGN_CENTER, FNTSMALL, ILI9341_CYAN, line);
    }
}

// ─── Diagnostics page ────────────────────────────────────────────────────────
//
// Layout (320×200 content area):
//   y=5..93  Two-column text data (5 rows × 22px)
//   y=113    Divider line
//   y=116    Section labels: MAIN  |  MON  |  AUX
//   y=130    First row of peripheral status icons (Vt / LF-AF)
//   y=170    Second row of peripheral status icons (Cu / CF-??)

static void pdl_draw_diagnostics_page(const PDLInfo *info)
{
    char line[48];
    const int16_t L = 5;
    const int16_t R = PDL_WIDTH - 5;
    int16_t y = 5;
    const int16_t ROW = 22;

    // Battery Voltage | Bus Voltage
    snprintf(line, sizeof(line), "BattV: %.1fV", info->battery_voltage / 10.0f);
    pdl_draw_text(L, y, MF_ALIGN_LEFT,  FNTSMALL, WHITE, line);
    snprintf(line, sizeof(line), "BusV: %.1fV", info->motor_bus_voltage);
    pdl_draw_text(R, y, MF_ALIGN_RIGHT, FNTSMALL, WHITE, line);
    y += ROW;

    // Battery Current | Bus Current
    snprintf(line, sizeof(line), "BattI: %.1fA", info->battery_current / 10.0f);
    pdl_draw_text(L, y, MF_ALIGN_LEFT,  FNTSMALL, WHITE, line);
    snprintf(line, sizeof(line), "BatPwr: %.1fkW", info->battery_power_kw / 100.0f);
    pdl_draw_text(R, y, MF_ALIGN_RIGHT, FNTSMALL, WHITE, line);
    y += ROW;

    // Battery Temp | Motor Temp
    snprintf(line, sizeof(line), "BattT: %dC", info->battery_high_temp);
    pdl_draw_text(L, y, MF_ALIGN_LEFT,  FNTSMALL, WHITE, line);
    snprintf(line, sizeof(line), "MotorT: %.1fC", info->motor_temp);
    pdl_draw_text(R, y, MF_ALIGN_RIGHT, FNTSMALL, WHITE, line);
    y += ROW;

    // Motor Velocity | CCL (only shown when charging)
    snprintf(line, sizeof(line), "MotorV: %.0fV", info->motor_velocity);
    pdl_draw_text(L, y, MF_ALIGN_LEFT, FNTSMALL, WHITE, line);
    snprintf(line, sizeof(line), "CCL: %dA", info->pack_ccl);
    pdl_draw_text(R, y, MF_ALIGN_RIGHT, FNTSMALL, WHITE, line);
    y += ROW;

    // Divider between text section and peripheral status section
    GFX_drawFastHLine(0, 103, PDL_WIDTH, WHITE);

    // Section labels
    pdl_draw_text(25,  106, MF_ALIGN_CENTER, FNTSMALL, WHITE, "MAIN");
    pdl_draw_text(160, 106, MF_ALIGN_CENTER, FNTSMALL, WHITE, "MONITOR");
    pdl_draw_text(295, 106, MF_ALIGN_CENTER, FNTSMALL, WHITE, "AUX");

    // Peripheral status icons
    // Main LV (left, x=10), Monitor (center, x1=115/x2=205), Aux LV (right, x=310)
    pdl_draw_battery_stats<DistroDisplayMain>(MF_ALIGN_LEFT,  10,  130, info->main_status);
    pdl_draw_monitor_stats(MF_ALIGN_LEFT, MF_ALIGN_RIGHT, 105, 215, 130, info->monitor_status);
    pdl_draw_battery_stats<DistroDisplayAux> (MF_ALIGN_RIGHT, 310, 130, info->aux_status);
}

// ─── Critical fault detection and overlay ─────────────────────────────────────
// Priority: Isolation > Contactor > BMS Non-Operational > Aux Battery

enum class CriticalFault : uint8_t
{
    None       = 0,
    Isolation,
    Contactor,
    BmsNonOp,
    AuxBattery,
};

static CriticalFault pdl_get_critical_fault(const PDLInfo *info)
{
    using D1  = BmsDtcFlags1;
    using D21 = BmsDtcFlags21;
    using D22 = BmsDtcFlags22;
    using DA  = DistroDisplayAux;
    using DMi = DistroDisplayMisc;

    // 1. Isolation fault — highest priority
    if (info->bms_dtc_flags2_2 & sbit(D22::HighVoltageIsolationFault))
        return CriticalFault::Isolation;

    // 2. Contactor fault — power distro board sets this bit when it detects the fault
    if (!info->power_distro_stale && (info->monitor_status & sbit(DMi::ContactorFault)))
        return CriticalFault::Contactor;

    // 3. BMS non-operational (OBD-II codes P0A07, P0A09, P0A0B, P0A1F, P0A04, P0AC0, P0A0F, P0560, P0A05)
    {
        const uint8_t nonop1  = sbit(D1::DischargeLimitEnforcementFault) |  // P0A07
                                sbit(D1::InternalHardwareFault)           |  // P0A09
                                sbit(D1::InternalSoftwareFault);             // P0A0B
        const uint8_t nonop21 = sbit(D21::InternalCommunicationFault)    |  // P0A1F
                                sbit(D21::OpenWiringFault)                |  // P0A04
                                sbit(D21::CurrentSensorFault)             |  // P0AC0
                                sbit(D21::CellAsicFault);                    // P0A0F
        const uint8_t nonop22 = sbit(D22::RedundantPowerSupplyFault)     |  // P0560
                                sbit(D22::InputPowerSupplyFault);            // P0A05
        if ((info->bms_dtc_flags1   & nonop1)  ||
            (info->bms_dtc_flags2_1 & nonop21) ||
            (info->bms_dtc_flags2_2 & nonop22))
            return CriticalFault::BmsNonOp;
    }

    // 4. Aux battery fault — any aux error or aux hardware/comms fault
    {
        const uint8_t aux_err = sbit(DA::AuxOverVoltageError)   |
                                sbit(DA::AuxUnderVoltageError)   |
                                sbit(DA::AuxOverCurrentError)    |
                                sbit(DA::AuxUnderCurrentError);
        bool aux_errors   = (info->aux_status & aux_err) != 0;
        bool aux_hw_fault = (info->monitor_status &
                             (sbit(DMi::AuxHardwareDetectedFault) |
                              sbit(DMi::AuxPowerMonitorI2cError))) != 0;
        if (aux_errors || aux_hw_fault)
            return CriticalFault::AuxBattery;
    }

    return CriticalFault::None;
}

static constexpr int overlayMainY = 75;
static constexpr int overlayMainH = 75;
static constexpr int overlayDiagY = 0;
static constexpr int overlayDiagH = 104;

static CriticalFault s_latched_fault = CriticalFault::None;

// is_active=true  → fault currently present:     red   overlay, 2-line layout.
// is_active=false → fault cleared but latched:   yellow overlay, 3-line layout + "LATCHED".
static void pdl_draw_critical_overlay(CriticalFault fault, bool show_diagnostics, bool is_active)
{
    int16_t oy = show_diagnostics ? overlayDiagY : overlayMainY;
    int16_t oh = show_diagnostics ? overlayDiagH : overlayMainH;

    uint16_t bg_color   = is_active ? ILI9341_RED : ILI9341_YELLOW;
    uint16_t text_color = is_active ? WHITE        : ILI9341_BLACK;

    GFX_fillRect(0, oy, PDL_WIDTH, oh, bg_color);

    const char *fault_name = "";
    const char *action     = "";

    switch (fault)
    {
    case CriticalFault::Contactor:
        fault_name = "CONTACTOR FAULT";
        action     = "ESTOP + EXIT CAR, BATTERY LIVE";
        break;
    case CriticalFault::Isolation:
        fault_name = "ISOLATION FAULT";
        action     = "ESTOP + EXIT CAR, CHASSIS LIVE";
        break;
    case CriticalFault::BmsNonOp:
        fault_name = "BMS NON-OPERATIONAL";
        action     = "ESTOP + EXIT CAR, BMS ERROR";
        break;
    case CriticalFault::AuxBattery:
        fault_name = "AUX BATTERY FAULT";
        action     = "TURN OFF, AUX ERROR";
        break;
    default:
        return;
    }

    uint16_t old_bgcolor = bgcolor;
    bgcolor = bg_color;

    const int16_t line_h = 20;

    if (is_active) {
        // Two-line layout: fault name + action, centred with generous gap.
        const int16_t gap       = 12;
        const int16_t content_h = line_h + gap + line_h;
        int16_t y_name   = oy + (oh - content_h) / 2;
        int16_t y_action = y_name + line_h + gap;
        pdl_draw_text(PDL_WIDTH / 2, y_name,   MF_ALIGN_CENTER, FNTSMALL, text_color, fault_name);
        pdl_draw_text(PDL_WIDTH / 2, y_action, MF_ALIGN_CENTER, FNTSMALL, text_color, action);
    } else {
        // Three-line layout: fault name + action + "LATCHED", tighter gap to fit in overlay.
        const int16_t gap       = 6;
        const int16_t content_h = line_h + gap + line_h + gap + line_h;
        int16_t y_name   = oy + (oh - content_h) / 2;
        int16_t y_action = y_name + line_h + gap;
        int16_t y_status = y_action + line_h + gap;
        pdl_draw_text(PDL_WIDTH / 2, y_name,   MF_ALIGN_CENTER, FNTSMALL, text_color, fault_name);
        pdl_draw_text(PDL_WIDTH / 2, y_action, MF_ALIGN_CENTER, FNTSMALL, text_color, action);
        pdl_draw_text(PDL_WIDTH / 2, y_status, MF_ALIGN_CENTER, FNTSMALL, text_color, "LATCHED");
    }

    bgcolor = old_bgcolor;
}

// ─── Top-level draw ───────────────────────────────────────────────────────────

void pdl_draw(const PDLInfo *info)
{
    GFX_setClearColor(bgcolor);
    GFX_clearScreen();

    // Horizontal rule above warning banner
    GFX_drawFastHLine(0, PDL_MAIN_BOTTOM, PDL_WIDTH, WHITE);

    if (info->show_diagnostics)
        pdl_draw_diagnostics_page(info);
    else
        pdl_draw_main_page(info);

    // Get active faults before latched faults
    CriticalFault active = pdl_get_critical_fault(info);
    if (active != CriticalFault::None) {
        s_latched_fault = active;
        pdl_draw_critical_overlay(active, info->show_diagnostics, true);
    }
    else if (s_latched_fault != CriticalFault::None)
        pdl_draw_critical_overlay(s_latched_fault, info->show_diagnostics, false);

    // Warning banner (same on both pages)
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
