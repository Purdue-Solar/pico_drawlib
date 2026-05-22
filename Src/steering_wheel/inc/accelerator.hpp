#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico_canlib.hpp" 

// Drive states
enum DriveDirection : uint8_t {
    Neutral = 0,
    Forward = 1,
    Reverse = 2,
};

static constexpr int32_t motorPositiveRPM = 20000; 
static constexpr int32_t motorNegativeRPM = -20000;
static constexpr float pedalDeadzone = 0.05f; // 5% deadzone (can change if necessary)

class Accelerator {
public:
    void init();
    void update(pico_canlib &can);
    DriveDirection direction() const { return m_drive_state; }  // exposes state for main.cpp if needed

private:
    void update_drive_state();
    void update_pedal_value();
    void send_motor_command(pico_canlib &can);

    float          m_pedal_value = 0.0f;
    DriveDirection m_drive_state = DriveDirection::Neutral;
    //Motor velocity constants
};















