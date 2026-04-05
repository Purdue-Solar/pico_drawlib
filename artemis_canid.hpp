#pragma once
#include "pico/stdlib.h"

class artemis_canid
{
public:
    static constexpr uint32_t tempAndSOC = 0x200;
    static constexpr uint32_t currentLimit = 0x201;
    static constexpr uint32_t battDiagnostic = 0x202;
    static constexpr uint32_t setMotorCurrent = 0x402;
    static constexpr uint32_t motorVelocity = 0x403;
    static constexpr uint32_t heatSinkTemp = 0x40B;
    static constexpr uint32_t steeringWheel = 0x301;
};
