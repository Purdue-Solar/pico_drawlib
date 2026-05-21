#pragma once
#include "pico/stdlib.h"

enum class artemis_canid : uint32_t {
    tempAndSOC = 0x200,
    currentLimit = 0x201,
    battDiagnostic = 0x202,
    setMotorCurrent = 0x402,
    motorVelocity = 0x403,
    heatSinkTemp = 0x40B,
    steeringWheel = 0x301,
    motorCurrentCanID = 0x501u,
    motorVelocityCanID = 0x502u,
    powerDistroToSteering = 0x301u,
    steeringToPowerDistro = 0x701u
};

constexpr uint32_t canIDHelper(artemis_canid c)
{
    return static_cast<uint32_t>(c);
}
