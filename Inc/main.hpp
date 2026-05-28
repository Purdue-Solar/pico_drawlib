#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <cstring>
#include "pico/stdlib.h"
#include "pico_canlib.hpp"
#include "accelerator.hpp"
#include "features.hpp"
#include "artemis_canid.hpp"
#include "matrix.hpp"
#include "pdl.hpp"
/*
Bit positions for artemis_canid::steeringToPowerDistro payload. The CAN 
spreadsheet is the ultimate authority on the meaning of each bit.
*/ 
enum class Steering_wheelMsg : uint8_t 
{
    bitLeftLights = 0,
    bitRightLights = 1,
    bitHazard = 2,
    bitHorn = 3,
    bitCruiseEn = 4,
    bitCruiseUp = 5,
    bitCruiseDown = 6,
    bitBright = 7,
    bitRegen = 8,
    bitBrakeLights = 9,
};

/*
Shortcut for bitshift, accounts for fact that enumClass interprets data as class rather than actual type
*/
inline uint8_t sbit(Steering_wheelMsg b)
{
    return 1u << static_cast<uint8_t>(b);
}

struct WheelState 
{
    bool left_light  = false;
    bool right_light = false;
    bool hazards     = false;
    bool brights     = false;
    bool cruise      = false;
    bool horn        = false;
    bool cruise_up   = false;
    bool cruise_down = false;
    bool regen       = false;
    bool brake       = false;
};
static WheelState wheel;

// Steering Wheel → Power Distro
static void send_steering_wheel_can_state(WheelState wheel, pico_canlib * can)
{
    uint16_t bits = 0;

    if (wheel.left_light)  bits |= sbit(Steering_wheelMsg::bitLeftLights);
    if (wheel.right_light) bits |= sbit(Steering_wheelMsg::bitRightLights);
    if (wheel.hazards)     bits |= sbit(Steering_wheelMsg::bitHazard);
    if (wheel.horn)     bits |= sbit(Steering_wheelMsg::bitHorn);

    if constexpr (FEAT_CRUISE_CONTROL) {
        if (wheel.cruise)      bits |= sbit(Steering_wheelMsg::bitCruiseEn);
        if (wheel.cruise_up)   bits |= sbit(Steering_wheelMsg::bitCruiseUp);
        if (wheel.cruise_down) bits |= sbit(Steering_wheelMsg::bitCruiseDown);
    }
    if constexpr (FEAT_BRIGHTS) {
        if (wheel.brights) bits |= sbit(Steering_wheelMsg::bitBright);
    }
    if constexpr (FEAT_REGEN) {
        if (wheel.regen) bits |= sbit(Steering_wheelMsg::bitRegen);
    }
    if constexpr (FEAT_BRAKE_PIN) {
        if (wheel.brake) bits |= sbit(Steering_wheelMsg::bitBrakeLights);
    }

    uint8_t data[2] = { (uint8_t)(bits & 0xFFu), (uint8_t)(bits >> 8) };
    can->transmitCAN(XL2515::TX_BUFFER_SEL::TX0, canIDHelper(artemis_canid::steeringToPowerDistro), false, data, 2, XL2515::PRIORITY::Highest);
}

// Parse Power Distro → Steering Wheel and update status flags
static void process_power_distro_status(uint8_t *data, uint8_t length, PDLInfo* info)
{
    if (length < 3) {
        info -> monitor_status = 255; // This indicates that everything is in the error state
        info -> main_status = 255;
        info -> aux_status = 255;
        return;
    }

    info -> monitor_status = data[PowerDistroMsg::BYTE_MONITOR];
    info -> main_status = data[PowerDistroMsg::BYTE_MAIN];
    info -> aux_status = data[PowerDistroMsg::BYTE_AUX];
}

/*
A note on toggle vs momentary buttons:
Toggle buttons are toggled on the positive edge of the button pressed.
Momentary buttons are on whenever the button is being pressed. This seems odd for a momentary 
button, but this makes more sense because packets can be dropped on the CAN bus - the recipient
device (like powerDistro) should be in charge of edge detection. 
*/
static void on_left_light(void)  { wheel.left_light  = !wheel.left_light;  }  // SW01
static void on_right_light(void) { wheel.right_light = !wheel.right_light; }  // SW02
static void on_hazards(void)     { wheel.hazards     = !wheel.hazards;     }  // SW03
static void on_brights(void) { if constexpr (FEAT_BRIGHTS) wheel.brights = !wheel.brights; } // SW04 
static void on_cruise_en(void) { if constexpr (FEAT_CRUISE_CONTROL) wheel.cruise = !wheel.cruise; } // SW05
static void on_cruise_up(void)   { (void)0; }  // SW06 – momentary; sent as bit on artemis_canid::steeringToPowerDistro
static void on_cruise_down(void) { (void)0; }  // SW07 – momentary; sent as bit on artemis_canid::steeringToPowerDistro
static void on_horn(void)        { (void)0; }  // SW08 – horn bit held high while pressed; read from button_pressed in main loop
static void on_ptt(void)         { (void)0; }  // SW09 – push-to-talk (no artemis_canid::steeringToPowerDistro field; reserve for radio if needed)
static void on_regen(void)       { (void)0; }  // SW10 – momentary; sent as bit on artemis_canid::steeringToPowerDistro