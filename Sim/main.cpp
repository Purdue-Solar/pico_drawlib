#include "artemis_canid.hpp"
#include "ili9341.h"
#include "gfx.h"
#include "fonts.h"
#include "mcufont.h"
#include "icons.h"
#include "pdl.hpp"

#ifdef SIMULATION
#include "picosdk_sim.h"
#include "raylib.h"
#include <math.h>
#else
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#endif



#include <stdio.h>

/* Returns true for roughly half each period, creating a square wave.
   Used to drive time-varying demo values. */
static bool periodic(double offset, double frequency) {
    double time = GetTime() + offset;
    return fmod(time * frequency, 1.0) < 0.5;
}

/* Build a plausible PDLInfo for the visual demo.
   Values change over time so both display pages can be exercised. */
static PDLInfo get_info(void) {
    PDLInfo info = {0};
    double t = GetTime();

    /* BMS Safety Critical (0x200)
       bit 0 = DischargeRelayEnabled, bit 6 = IsReadyStatus */
    info.bms_relay_state1 = sbit(BmsRelayState1::DischargeRelayEnabled) | (1u << 6);
    info.battery_soc      = 85;

    /* BMS Pack Voltage and Energy (0x201) — raw BMS scaling */
    info.battery_voltage  = 1200;   /* /10.0f → 120.0 V */
    info.battery_current  = (uint16_t)(500 + 80 * sin(t * 0.4));  /* /10.0f → ~50 A */
    info.battery_power_kw = (uint16_t)(600 + 80 * sin(t * 0.4));  /* /100.0f → ~6.0 kW */

    /* BMS Temperature / Current Limits / Power (0x202) */
    info.pack_dcl          = 120;   /* 120 A discharge limit */
    info.pack_ccl          = 40;    /* 40 A charge limit */
    info.battery_high_temp = 35;    /* 35 °C */

    /* Power Distro Display (0x300) — all clear */
    info.monitor_status = 0;
    info.main_status    = 0;
    info.aux_status     = 0;

    /* Motor Controller (0x402, 0x403, 0x40B) */
    info.motor_bus_voltage = 118.5f;
    info.motor_bus_current = (float)(50.0 + 8.0 * sin(t * 0.4));
    info.vehicle_velocity  = (float)(25.0 + 5.0 * sin(t * 0.15));
    info.motor_velocity    = (float)(3200.0 + 600.0 * sin(t * 0.15));
    info.motor_temp        = 45.0f;
    info.heat_sink_temp    = 55.0f;

    /* Demonstrate a periodic CAN-stale warning (stale for ~10 s every 20 s) */
    info.mc_speed_stale = periodic(0.0, 0.05);

    /* Alternate pages every 8 seconds so both are visible in the demo */
    info.show_diagnostics = (fmod(t, 16.0) >= 8.0);

    /* Power hold: active during seconds 4–12 of each 16 s cycle; setpoint ramps 10→19 A */
    double cycle = fmod(t, 16.0);
    info.power_hold_enabled = (cycle >= 4.0 && cycle < 12.0);
    info.power_hold_amps    = (uint8_t)(10 + (uint8_t)(9.0 * (cycle - 4.0) / 8.0));

    /* ── Critical fault injection ───────────────────────────────────────────────
     * Cycle: 5 s each fault, then 5 s clean.  The page-flip above runs on its
     * own 16 s cadence, so each fault is visible on both the main and diag page
     * naturally without extra coordination.
     *
     * The contactor fault is set directly by the power distro board (ContactorFault bit
     * in monitor_status); it clears instantly when the phase ends.  Latching means
     * cleared faults remain visible (yellow) in subsequent phases, so the 5 s "clean"
     * window at the end shows the last latched fault. */
    {
        double fault_t = fmod(t, 25.0);

        if (fault_t < 5.0) {
            /* Phase 1 — isolation fault */
            info.bms_dtc_flags2_2 |= sbit(BmsDtcFlags22::HighVoltageIsolationFault);
        } else if (fault_t < 10.0) {
            /* Phase 2 — contactor fault: power distro board reports contactor fault */
            info.monitor_status |= sbit(DistroDisplayMisc::ContactorFault);
        } else if (fault_t < 15.0) {
            /* Phase 3 — BMS non-operational (P0A09 internal hardware fault) */
            info.bms_dtc_flags1 |= sbit(BmsDtcFlags1::InternalHardwareFault);
        } else if (fault_t < 20.0) {
            /* Phase 4 — aux battery fault (aux overvoltage error) */
            info.aux_status |= sbit(DistroDisplayAux::AuxOverVoltageError);
        }
        /* Phase 5 (20–25 s): no injected fault — latched fault from phase 4 shown yellow */
    }

    return info;
}

int main(void) {
    stdio_init_all();

    LCD_setPins(20, 17, 21, 18, 19);
    LCD_setSPIperiph(spi0);
    LCD_initDisplay();
    LCD_setRotation(1);
    GFX_createFramebuf();

#ifdef SIMULATION
    LCDSim_InitWindow();

    while (!LCDSim_WindowShouldClose())
#else
    while (1)
#endif
    {
        PDLInfo info = get_info();
        pdl_draw(&info);  /* renders and flushes the framebuffer */

#ifdef SIMULATION
        LCDSim_Redraw();  /* blit framebuffer → raylib window */
#endif
    }

    GFX_destroyFramebuf();

#ifdef SIMULATION
    LCDSim_CloseWindow();
#endif

    return 0;
}
