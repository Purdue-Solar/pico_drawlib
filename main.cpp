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

/*
Bit positions for artemis_canid::steeringToPowerDistro payload. The CAN 
spreadsheet is the ultimate authority on the meaning of each bit.
*/ 
enum class Steering_wheelMsg : uint8_t {
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
uint8_t sbit(Steering_wheelMsg b)
{
    return 1u << static_cast<uint8_t>(b);
}

// Layout of the Power Distro → Steering Wheel CAN message
struct PowerDistroMsg {
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

uint8_t mbit(PowerDistroMsg::MonitorBit b)
{
    return 1u << static_cast<uint8_t>(b);
}

// Last status from Power Distro
static bool status_aux_fault  = false;
static bool status_main_fault = false;

struct WheelState {
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

pico_canlib can = pico_canlib();

// Steering Wheel → Power Distro
static void send_steering_wheel_can_state(WheelState wheel)
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
    can.transmitCAN(XL2515::TX_BUFFER_SEL::TX0, canIDHelper(artemis_canid::steeringToPowerDistro), false, data, 2, XL2515::PRIORITY::Highest);
}

// Parse Power Distro → Steering Wheel and update status flags
static void process_power_distro_status(uint8_t *data, uint8_t length)
{
    if (length < 3) {
        status_main_fault = 1;
        status_aux_fault = 1;
        // Make it so that a more sophisticated error than this is returned
        return;
    }

    using MB = PowerDistroMsg::MonitorBit;
    status_main_fault = (data[PowerDistroMsg::BYTE_MONITOR] | mbit(MB::DcdcInvalid) | mbit(MB::MainMonitorError)) && (data[PowerDistroMsg::BYTE_MAIN] & 0x0F);
    status_aux_fault  = (data[PowerDistroMsg::BYTE_MONITOR] | mbit(MB::AuxInvalid)  | mbit(MB::AuxMonitorError))  && (data[PowerDistroMsg::BYTE_AUX]  & 0x0F);
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

static constexpr uint32_t NumRows = 3;
static constexpr uint32_t NumCols = 4;
constexpr uint8_t rowPins[NumRows] = {16, 17, 13};
constexpr uint8_t colPins[NumCols] = {15, 14, 18, 19};

int main()
{
    pico_canlib::status errorCode;
    errorCode = can.init();
    fprintf(stdout, "Init Code %d\n", errorCode);

    if (errorCode != pico_canlib::status::SUCCESS)
        fprintf(stdout, "Failed Startup\n");

    gpio_init(LEDPin);
    gpio_set_dir(LEDPin, GPIO_OUT);

    Accelerator accelerator;
    accelerator.init();

    if constexpr (FEAT_BRAKE_PIN) {
        gpio_init(brakeInputPin);
        gpio_set_dir(brakeInputPin, GPIO_IN);
        gpio_pull_down(brakeInputPin);
    }

    // [row][col] — must match the physical wiring layout in rowPins/colPins above
    Matrix matrix(0, 1, 5, rowPins, NumRows, colPins, NumCols, {{
        { on_left_light, on_right_light, on_hazards,    on_brights    },  /* row 0 */
        { on_cruise_en,  on_cruise_up,   on_cruise_down, nullptr      },  /* row 1 */
        { on_horn,       on_ptt,         on_regen,       nullptr      },  /* row 2 */
    }});
    matrix.matrix_init();
    matrix.keypad_init_timer();

    while (true) {
        // Read momentary buttons directly from the matrix (held state)
        wheel.horn        = matrix.button_pressed[2][0];
        wheel.cruise_up   = matrix.button_pressed[1][1];
        wheel.cruise_down = matrix.button_pressed[1][2];
        wheel.regen       = matrix.button_pressed[2][2];
        if constexpr (FEAT_BRAKE_PIN)
            wheel.brake = gpio_get(brakeInputPin) != 0;

        send_steering_wheel_can_state(wheel);

        // Process received CAN (Power Distro → Steering Wheel artemis_canid::powerDistroToSteering).
        {
            // Buffer layout per new receiveCAN API: [0–3] id, [4] data length, [5–12] data
            uint8_t rx_buf[13];
            if (!(int)can.receiveCAN(rx_buf, 4, 8)) {
                uint32_t id;
                memcpy(&id, rx_buf, 4);
                if (id == canIDHelper(artemis_canid::powerDistroToSteering)) {
                    // Go to rx_buf + 5 to start at data, use rx_buf[4] to pass data length
                    process_power_distro_status(rx_buf + 5, rx_buf[4]);
                }
            }
        }

        // LED: heartbeat when idle; blink when cruise on; fast blink when main fault
        static uint32_t led_tick = 0;
        led_tick++;
        bool led_on;
        if (status_main_fault)
            led_on = (led_tick % 6) < 3;
        else if (wheel.cruise)
            led_on = ((led_tick / 10) % 2 == 0);
        else
            led_on = (led_tick % 50 < 45);
        gpio_put(LEDPin, led_on ? 1 : 0);

        accelerator.update(can);
    }
}
