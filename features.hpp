#pragma once

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
static constexpr uint8_t numRows = 3;
static constexpr uint8_t numColumns = 4;
static constexpr uint8_t scanDelayMS = 5;

// Row pins (inputs)
static const uint row_pins[numRows] = { 16, 17, 13 };
// Column pins (outputs)
static const uint col_pins[numColumns] = { 15, 14, 18, 19 };

// Function called when a button is pressed. NULL = no button
typedef void (*button_handler_t)(void);