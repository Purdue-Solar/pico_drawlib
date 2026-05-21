#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <cstring>
#include "pico/stdlib.h"
#include "pico_canlib.hpp"
#include "accelerator.hpp"
#include "features.hpp"
#include "artemis_canid.hpp"

// Bit positions for artemis_canid::steeringToPowerDistro payload
enum class SteeringWheelMsg : uint8_t {
    bitLeftLights = 0,
    bitRightLights = 1,
    bitHazard = 2,
    bitHorn = 3,
    bitCruiseEn = 255,
    bitCruiseUp = 255,
    bitCruiseDown = 255,
    bitBright = 255,
    bitRegen = 255,
    bitBrakeLights = 255,
};

uint8_t sbit(SteeringWheelMsg b)
{
    return 1u << static_cast<uint8_t>(b);
}

// Layout of the Power Distro → Steering Wheel CAN message
struct PowerDistroMsg {
    static constexpr uint8_t BYTE_MONITOR = 0;
    static constexpr uint8_t BYTE_MAIN    = 1;
    static constexpr uint8_t BYTE_AUX     = 2;

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

// Cruise state
static bool g_cruise_enabled = false;

// Lights state
static bool g_left_light_on  = false;
static bool g_right_light_on = false;
static bool g_hazards_on     = false;
static bool g_brights_on     = false;

// Last status from Power Distro
static bool g_status_aux_fault  = false;
static bool g_status_main_fault = false;

pico_canlib g_can = pico_canlib();

// Steering Wheel → Power Distro
static void send_steering_wheel_can_state(bool cruise_up_pressed, bool cruise_down_pressed,
                                          bool horn_pressed, bool regen_pressed, bool brake_pressed)
{
    uint16_t bits = 0;

    if (g_left_light_on)  bits |= sbit(SteeringWheelMsg::bitLeftLights);
    if (g_right_light_on) bits |= sbit(SteeringWheelMsg::bitRightLights);
    if (g_hazards_on)     bits |= sbit(SteeringWheelMsg::bitHazard);
    if (horn_pressed)     bits |= sbit(SteeringWheelMsg::bitHorn);

    if constexpr (FEAT_CRUISE_CONTROL) {
        if (g_cruise_enabled)    bits |= sbit(SteeringWheelMsg::bitCruiseEn);
        if (cruise_up_pressed)   bits |= sbit(SteeringWheelMsg::bitCruiseUp);
        if (cruise_down_pressed) bits |= sbit(SteeringWheelMsg::bitCruiseDown);
    }
    if constexpr (FEAT_BRIGHTS) {
        if (g_brights_on) bits |= sbit(SteeringWheelMsg::bitBright);
    }
    if constexpr (FEAT_REGEN) {
        if (regen_pressed) bits |= sbit(SteeringWheelMsg::bitRegen);
    }
    if constexpr (FEAT_BRAKE_PIN) {
        if (brake_pressed) bits |= sbit(SteeringWheelMsg::bitBrakeLights);
    }

    uint8_t data[2] = { (uint8_t)(bits & 0xFFu), (uint8_t)(bits >> 8) };
    g_can.transmitCAN(XL2515::TX_BUFFER_SEL::TX0, canIDHelper(artemis_canid::steeringToPowerDistro), false, data, 2, XL2515::PRIORITY::Highest);
}

// Parse Power Distro → Steering Wheel and update status flags
static void process_power_distro_status(uint8_t *data, uint8_t length)
{
    if (length < 3)
        g_status_main_fault = 1;
        g_status_aux_fault = 1;
        // Make it so that a more sophisticated error than this is returned
    return;

    using MB = PowerDistroMsg::MonitorBit;
    g_status_main_fault = (data[PowerDistroMsg::BYTE_MONITOR] & mbit(MB::DcdcInvalid) & mbit(MB::MainMonitorError)) && (data[PowerDistroMsg::BYTE_MAIN] & 0x0F);
    g_status_aux_fault  = (data[PowerDistroMsg::BYTE_MONITOR] & mbit(MB::AuxInvalid)  & mbit(MB::AuxMonitorError))  && (data[PowerDistroMsg::BYTE_AUX]  & 0x0F);
}

// logic
static void on_left_light(void)  { g_left_light_on  = !g_left_light_on;  }  // SW01
static void on_right_light(void) { g_right_light_on = !g_right_light_on; }  // SW02
static void on_hazards(void)     { g_hazards_on     = !g_hazards_on;     }  // SW03
static void on_brights(void) { if constexpr (FEAT_BRIGHTS) g_brights_on = !g_brights_on; } // SW04 
static void on_cruise_en(void) { if constexpr (FEAT_CRUISE_CONTROL) g_cruise_enabled = !g_cruise_enabled; } // SW05
static void on_cruise_up(void)   { (void)0; }  // SW06 – momentary; sent as bit on artemis_canid::steeringToPowerDistro
static void on_cruise_down(void) { (void)0; }  // SW07 – momentary; sent as bit on artemis_canid::steeringToPowerDistro
static void on_horn(void)        { (void)0; }  // SW08 – momentary; sent as bit on artemis_canid::steeringToPowerDistro
static void on_ptt(void)         { (void)0; }  // SW09 – push-to-talk (no artemis_canid::steeringToPowerDistro field; reserve for radio if needed)
static void on_regen(void)       { (void)0; }  // SW10 – momentary; sent as bit on artemis_canid::steeringToPowerDistro

// Debounce: only trigger handlers on rising edge
static bool prev_pressed[numRows][numColumns] = {{false}};

// [row][col]. NULL = no button
static const button_handler_t button_handlers[numRows][numColumns] = {
    { on_left_light, on_right_light, on_hazards,    on_brights    },  /* row 0 */
    { on_cruise_en,  on_cruise_up,   on_cruise_down, NULL         },  /* row 1 */
    { on_horn,       on_ptt,         on_regen,       NULL         },  /* row 2 */
};

int main()
{
    pico_canlib::status errorCode;
    errorCode = g_can.init();
    fprintf(stdout, "Init Code %d\n", errorCode);

    if (errorCode != pico_canlib::status::SUCCESS)
        fprintf(stdout, "Failed Startup\n");

    for (int r = 0; r < numRows; r++) {
        gpio_init(row_pins[r]);
        gpio_set_dir(row_pins[r], GPIO_IN);
        gpio_pull_down(row_pins[r]);
    }
    for (int c = 0; c < numColumns; c++) {
        gpio_init(col_pins[c]);
        gpio_set_dir(col_pins[c], GPIO_OUT);
        gpio_put(col_pins[c], 0);
    }
    gpio_init(LEDPin);
    gpio_set_dir(LEDPin, GPIO_OUT);
    Accelerator accelerator;
    accelerator.init();

    if constexpr (FEAT_BRAKE_PIN) {
        gpio_init(brakeInputPin);
        gpio_set_dir(brakeInputPin, GPIO_IN);
        gpio_pull_down(brakeInputPin);
    }

    while (true) {
        bool button_pressed[numRows][numColumns] = {{false}};

        for (int col = 0; col < numColumns; col++) {
            for (int c = 0; c < numColumns; c++) {
                gpio_put(col_pins[c], (c == col));
            }
            sleep_ms(scanDelayMS);

            for (int row = 0; row < numRows; row++) {
                bool now = gpio_get(row_pins[row]) != 0;
                button_pressed[row][col] = now;
                if (now && !prev_pressed[row][col]) {
                    button_handler_t handler = button_handlers[row][col];
                    if (handler != NULL)
                        handler();
                }
                prev_pressed[row][col] = now;
            }
        }

        bool brake = false;
        if constexpr (FEAT_BRAKE_PIN) {
            brake = gpio_get(brakeInputPin) != 0;
        }

        send_steering_wheel_can_state(
            button_pressed[1][1],
            button_pressed[1][2],
            button_pressed[2][0],
            button_pressed[2][2],
            brake
        );

        // Process received CAN (Power Distro → Steering Wheel artemis_canid::powerDistroToSteering).
        {
            // Buffer layout per new receiveCAN API: [0–3] id, [4] data length, [5–12] data
            uint8_t rx_buf[13];
            if (!(int)g_can.receiveCAN(rx_buf, 4, 8)) {
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
        if (g_status_main_fault)
            led_on = (led_tick % 6) < 3;
        else if (g_cruise_enabled)
            led_on = ((led_tick / 10) % 2 == 0);
        else
            led_on = (led_tick % 50 < 45);
        gpio_put(LEDPin, led_on ? 1 : 0);

        accelerator.update(g_can);
    }
}
