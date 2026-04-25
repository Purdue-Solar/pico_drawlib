#ifndef PDL_H
#define PDL_H

#include "mcufont.h"

/*

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

typedef struct PDLInfo
{
    // ASSUMPTION: It is in celsius
    uint8_t battery_temperature;

    // SOC = state of charge (battery percentage?)
    // ASSUMPTION: Range = 0 - 100
    uint8_t battery_soc;

    uint32_t motor_current;
    uint32_t motor_velocity;

    bool aux_over_voltage;
    bool aux_under_voltage;
    bool aux_over_current;
    bool aux_current_warning;

    bool main_over_voltage;
    bool main_under_voltage;
    bool main_over_current;
    bool main_current_warning;
    // uint8_t aux_condition; // 4 bits
    bool main_valid; // DC valid
    bool aux_valid;

    // uint32_t velocity;

    // uint32_t motor_temperature;

    // unsigned int aux_current;
    // unsigned int aux_voltage;
} PDLInfo;

#define PDL_WIDTH 320
#define PDL_HEIGHT 240
#define PDL_WARNING_HEIGHT 40
#define PDL_CENTERPANEL_WIDTH 160

#define PDL_CENTERPANEL_LEFT ((PDL_WIDTH - PDL_CENTERPANEL_WIDTH) / 2.0)
#define PDL_CENTERPANEL_RIGHT (PDL_CENTERPANEL_LEFT + PDL_CENTERPANEL_WIDTH)
#define PDL_MAIN_BOTTOM (PDL_HEIGHT - PDL_WARNING_HEIGHT)

#ifdef __cplusplus
extern "C"
{
#endif

    void pdl_draw(const PDLInfo *info);

#ifdef __cplusplus
}
#endif

#endif // !PDL_H
