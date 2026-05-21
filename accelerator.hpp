#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico_canlib.hpp"

// Input pins
static constexpr uint8_t FNRForwardPin = 6;
static constexpr uint8_t FNRReversePin = 7;
static constexpr uint8_t AcceleratorPin = 26;  // GPIO26_ADC0

// Motor CAN IDs
static constexpr uint32_t motorCurrentCanID = 0x501u;
static constexpr uint32_t motorVelocityCanID = 0x502u;

// Drive states
enum DriveDirection : uint8_t {
        driveNeutral = 0,
        driveForward = 1,
        driveReverse = 2,
};

//Motor velocity constants
static constexpr int32_t motorPositiveRPM = 20000; 
static constexpr int32_t motorNegativeRPM = -20000;

static constexpr float pedalDeadzone = 0.05f; // 5% deadzone (can change if necessary)
//intiializing pins and updating acceklerator
void accelerator_init(void);
void accelerator_update(pico_canlib &can);
















