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
// Layout of the Power Distro → Steering Wheel CAN message
struct PowerDistroMsg 
{
    static constexpr uint8_t BYTE_MONITOR = 0; // 0th byte
    static constexpr uint8_t BYTE_MAIN    = 1; // 1st byte
    static constexpr uint8_t BYTE_AUX     = 2; // 2nd byte

    // Bit positions within the monitor byte
    enum class MonitorBit : uint8_t {
        DcdcInvalid      = 0,
        AuxInvalid       = 1,
        MainMonitorError = 2,
        AuxMonitorError  = 3,
    };

    // Shared bit layout for the MAIN and AUX output bytes
    enum class OutputBit : uint8_t {
        VoltageHighError = 0,
        VoltageLowError  = 1,
        CurrentHighError = 2,
        CurrentLowError  = 3,
        VoltageHighWarn  = 4,
        VoltageLowWarn   = 5,
        CurrentHighWarn  = 6,
        CurrentLowWarn   = 7,
    };
};

inline uint8_t mbit(PowerDistroMsg::MonitorBit b)
{
    return 1u << static_cast<uint8_t>(b);
}

inline uint8_t obit(PowerDistroMsg::OutputBit b)
{
    return 1u << static_cast<uint8_t>(b);
}

typedef struct PDLInfo
{
    // ASSUMPTION: It is in celsius
    uint8_t battery_temperature;

    // SOC = state of charge (battery percentage?)
    // ASSUMPTION: Range = 0 - 100
    uint8_t battery_soc;

    uint32_t motor_current;
    uint32_t motor_velocity;

    uint8_t monitor_status;
    uint8_t main_status;
    uint8_t aux_status;
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
