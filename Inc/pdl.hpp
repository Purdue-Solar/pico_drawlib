#ifndef PDL_H
#define PDL_H

#include "mcufont.h"
#include "hardware/spi.h"

/*
Outdated sry
|---------------------------------320--------------------------------|

|--------80--------|--------------160-------------|--------80--------|

----------------------------------------------------------------------  -  -
| MAIN             |                              |             AUX  |  |  |
|                  |                              |                  |  |  |
|                  |                              |                  |  |  |
|                  |                              |                  |  2  |
|                  |                              |                  |  0  |
|                  |                              |                  |  0  |
|                  |                              |                  |  |  |
|                  |                              |                  |  |  2
|                  |                              |                  |  |  4
|                  |                              |                  |  |  0
|                  |                              |                  |  |  |
|                  |                              |                  |  |  |
|                  |                              |                  |  |  |
|--------------------------------------------------------------------|  -  |
| Warn |                                Warning                      |  4  |
| Icon |                                or Error Message             |  0  |
----------------------------------------------------------------------  -  -

|--40--|----------------------------280------------------------------|
*/
void pdl_init(spi_inst_t *spi, uint dc, uint cs, uint rst, uint sck, uint mosi);


typedef struct PDLInfo
{
    // ── BMS Safety Critical (0x200) ─────────────────────────────────
    uint8_t battery_soc;       // Pack SOC
    uint8_t bms_dtc_flags1;    // DTC Flags 1
    uint8_t bms_dtc_flags2_1;  // DTC Flags 2.1
    uint8_t bms_dtc_flags2_2;  // DTC Flags 2.2 — isolation fault at BmsDtcFlags22::HighVoltageIsolationFault
    uint8_t bms_relay_state1;  // Relay State 1 — used for vehicle state
    uint8_t bms_relay_state2;  // Relay State 2 — used for vehicle state

    // ── BMS Pack Voltage and Energy (0x201) ─────────────────────────
    uint16_t battery_power_kw;  // Pack kW Power (raw, BMS-specific scaling)
    uint16_t battery_current;   // Pack Current (raw, BMS-specific scaling)
    uint16_t battery_voltage;   // Pack Voltage (raw, BMS-specific scaling)

    // ── BMS Temperature / Current Limits / Power (0x202) ────────────
    uint8_t  pack_dcl;               // Pack Discharge Current Limit
    uint8_t  pack_ccl;               // Pack Charge Current Limit (shown on diagnostics if charging)
    uint8_t  battery_high_temp;      // Highest thermistor temperature (°C)
    uint16_t current_limits_status;  // Current limits status bitfield

    // ── Power Distro Display (0x300) ────────────────────────────────
    uint8_t monitor_status;  // power monitor flags — bit 4 repurposed as precharge active
    uint8_t main_status;     // main LV output flags
    uint8_t aux_status;      // aux LV output flags

    // ── Motor Controller Errors (0x401) ─────────────────────────────
    uint8_t mc_limit_flags;   // limit flags — "current limit status" (lowest 8 bits of MC status word)
    uint8_t mc_error_flags1;  // error flags 1 — motor fault bits 16-24
    uint8_t mc_error_flags2;  // error flags 2 — motor fault bits 24-32

    // ── Motor Bus Voltage / Current (0x402) ─────────────────────────
    float motor_bus_voltage;  // Bus Voltage (V, IEEE 754)
    float motor_bus_current;  // Bus Current (A, IEEE 754)

    // ── Motor Speed (0x403) ─────────────────────────────────────────
    float motor_velocity;    // Motor shaft velocity (RPM, IEEE 754)
    float vehicle_velocity;  // Vehicle velocity (m/s or rpm, IEEE 754) — "Vehicle Speed" on main page

    // ── Motor Temperature (0x40B) ────────────────────────────────────
    float motor_temp;      // Motor temperature (°C, IEEE 754)
    float heat_sink_temp;  // Heat sink temperature (°C, IEEE 754) — "Heatsink Temperature" on main page

    // ── CAN receive freshness flags ──────────────────────────────────
    // Set true when the most recent receive for that message failed the length check.
    // Data fields above are left unchanged when stale is set true.
    bool bms_safety_stale;
    bool bms_voltage_stale;
    bool bms_power_stale;
    bool power_distro_stale;
    bool mc_errors_stale;
    bool mc_bus_stale;
    bool mc_speed_stale;
    bool mc_temp_stale;

    // ── UI state ─────────────────────────────────────────────────────
    bool show_diagnostics;  // toggled by button press; selects diagnostics page over main page
} PDLInfo;

#define PDL_WIDTH 320
#define PDL_HEIGHT 240
#define PDL_WARNING_HEIGHT 40
#define PDL_CENTERPANEL_WIDTH 160

#define PDL_CENTERPANEL_LEFT ((PDL_WIDTH - PDL_CENTERPANEL_WIDTH) / 2.0)
#define PDL_CENTERPANEL_RIGHT (PDL_CENTERPANEL_LEFT + PDL_CENTERPANEL_WIDTH)
#define PDL_MAIN_BOTTOM (PDL_HEIGHT - PDL_WARNING_HEIGHT)

void pdl_draw(const PDLInfo *info);

#endif // !PDL_H