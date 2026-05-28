#include "ili9341.h"
#include "multicore.hpp"

#ifdef SIMULATION
#include "picosdk_sim.h"
#else
extern "C"
{
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "gfx.h"
}
#endif

void core1_entry()
{
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
        PDLInfo local_info;

        mutex_enter_blocking(&info_mutex);
        local_info = sharedInfo;        // struct copy — takes microseconds
        mutex_exit(&info_mutex);

        pdl_draw(&local_info);           
        #ifdef SIMULATION
        LCDSim_Redraw();
        #endif
    }
    #ifdef SIMULATION
    LCDSim_CloseWindow();
    #endif
}