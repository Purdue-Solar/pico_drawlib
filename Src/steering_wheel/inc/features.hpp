#pragma once
#include <array>
#include <cstdint>

constexpr bool FEAT_BRAKE_PIN      = false;
constexpr bool FEAT_CRUISE_CONTROL = false;
constexpr bool FEAT_REGEN          = false;
constexpr bool FEAT_BRIGHTS        = false;

// Input pins
static constexpr uint8_t FNRForwardPin = 6;
static constexpr uint8_t FNRReversePin = 7;
static constexpr uint8_t AcceleratorPin = 26;  // GPIO26_ADC0
static constexpr uint8_t brakeInputPin = 255; // Not currently used

// Output pin
static constexpr uint8_t LEDPin = 25;

static constexpr uint32_t NumRows = 3;
static constexpr uint32_t NumCols = 4;
constexpr uint8_t rowPins[NumRows] = {16, 17, 13};
constexpr uint8_t colPins[NumCols] = {15, 14, 18, 19};