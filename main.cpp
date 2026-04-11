#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico_canlib.hpp"
#include "accelerator.hpp"

// Matrix configuration
#define NUM_ROWS    3
#define NUM_COLUMNS 4
#define SCAN_DELAY_MS 5
#define LED_PIN 25

// If brake pedal is wired to steering wheel; else 255 = disabled
#define BRAKE_PIN_DISABLED 255
#define BRAKE_INPUT_PIN BRAKE_PIN_DISABLED

// Power Distro → Steering Wheel
#define POWER_DISTRO_TO_STEERING_WHEEL_ID 0x301u

// Row pins (inputs)
static const uint row_pins[NUM_ROWS] = { 16, 17, 13 };

// Column pins (outputs)
static const uint col_pins[NUM_COLUMNS] = { 15, 14, 18, 19 };

// Function called when a button is pressed. NULL = no button
typedef void (*button_handler_t)(void);



// Steering Wheel → Power Distro
#define STEERING_WHEEL_TO_POWER_DISTRO_ID  0x701u

// Bit positions for 0x701 payload
#define BIT_LEFT_LIGHTS    6
#define BIT_RIGHT_LIGHTS   7
#define BIT_HAZARD         8
#define BIT_CRUISE_EN      9
#define BIT_CRUISE_UP      10
#define BIT_CRUISE_DOWN    11
#define BIT_HORN           12
#define BIT_BRIGHT         13
#define BIT_REGEN          14
#define BIT_BRAKE_LIGHTS   15

// Bit positions for 0x801 (Power Distro → Steering Wheel)
#define STATUS_BIT_CRUISE            1
#define STATUS_BIT_CRUISE_UP        2
#define STATUS_BIT_CRUISE_DOWN      3
#define STATUS_BIT_REGEN            4
#define STATUS_BIT_AUX_CONDITION    7
#define STATUS_BIT_MAIN_CURRENT_WARN 8
#define STATUS_BIT_MAIN_OVERCURRENT  9
#define STATUS_BIT_MAIN_UNDERVOLT   10
#define STATUS_BIT_MAIN_OVERVOLT    11
#define STATUS_BIT_AUX_CURRENT_WARN 12
#define STATUS_BIT_AUX_OVERCURRENT  13
#define STATUS_BIT_AUX_UNDERVOLT    14
#define STATUS_BIT_AUX_OVERVOLT    15

// Cruise state
static bool g_cruise_enabled = false;

// Lights state
static bool g_left_light_on     = false;
static bool g_right_light_on    = false;
static bool g_hazards_on        = false;
static bool g_brights_on        = false;

// Last status from Power Distro (0x801) 
static bool g_status_regen_ok       = false;
static bool g_status_cruise_ok      = false;
static bool g_status_aux_ok         = false;
static bool g_status_main_fault      = false;

pico_canlib g_can;


// Steering Wheel → Power Distro: 0x701 
static void send_steering_wheel_can_state(bool cruise_up_pressed, bool cruise_down_pressed,
                                          bool horn_pressed, bool regen_pressed, bool brake_pressed)
{
    uint16_t bits = 0;

    if (g_left_light_on || g_hazards_on)
        bits |= (1u << BIT_LEFT_LIGHTS);
    if (g_right_light_on || g_hazards_on)
        bits |= (1u << BIT_RIGHT_LIGHTS);
    if (g_hazards_on)
        bits |= (1u << BIT_HAZARD);
    if (g_cruise_enabled)
        bits |= (1u << BIT_CRUISE_EN);
    if (cruise_up_pressed)
        bits |= (1u << BIT_CRUISE_UP);
    if (cruise_down_pressed)
        bits |= (1u <<  BIT_CRUISE_DOWN);
    if (horn_pressed)
        bits |= (1u << BIT_HORN);
    if (g_brights_on)
        bits |= (1u << BIT_BRIGHT);
    if (regen_pressed)
        bits |= (1u << BIT_REGEN);
    if (brake_pressed)
        bits |= (1u << BIT_BRAKE_LIGHTS);

    uint8_t data[2] = { (uint8_t)(bits & 0xFFu), (uint8_t)(bits >> 8) };
    g_can.transmitCAN(XL2515::TX_BUFFER_SEL::TX0, MOTOR_CURRENT_CAN_ID, false, data, 2, XL2515::PRIORITY::Highest);
    //xl2515_send(STEERING_WHEEL_TO_POWER_DISTRO_ID, data, 2);
}   

// Parse Power Distro → Steering Wheel (0x801) and update status flags
static void process_power_distro_status(uint8_t *data, uint8_t len)
{
    if (len < 2)
        return;
    uint16_t bits = (uint16_t)data[0] | ((uint16_t)data[1] << 8);

    g_status_regen_ok   = !(bits & (1u << STATUS_BIT_REGEN));       
    g_status_cruise_ok  = (bits & (1u << STATUS_BIT_CRUISE)) != 0;
    g_status_aux_ok     = (bits & (1u << STATUS_BIT_AUX_CONDITION)) != 0;
    g_status_main_fault = (bits & ((1u << STATUS_BIT_MAIN_CURRENT_WARN) | (1u << STATUS_BIT_MAIN_OVERCURRENT) |
                                   (1u << STATUS_BIT_MAIN_UNDERVOLT) | (1u << STATUS_BIT_MAIN_OVERVOLT))) != 0;
}

// logic
static void on_left_light(void)    // SW01
{
    g_left_light_on = !g_left_light_on;
}

static void on_right_light(void)   // SW02
{
    g_right_light_on = !g_right_light_on;
}

static void on_hazards(void)       // SW03
{
    g_hazards_on = !g_hazards_on;
}

static void on_brights(void)       // SW04
{
    g_brights_on = !g_brights_on;
}

static void on_cruise_en(void)     // SW05
{
    g_cruise_enabled = !g_cruise_enabled;
}

static void on_cruise_up(void)     // SW06 – momentary; sent as bit on 0x701
{
    (void)0;
}

static void on_cruise_down(void)   // SW07 – momentary; sent as bit on 0x701
{
    (void)0;
}

static void on_horn(void)          // SW08 – momentary; sent as bit on 0x701
{
    (void)0;
}

static void on_ptt(void)           // SW09 – push-to-talk (no 0x701 field; reserve for radio if needed)
{
    (void)0;
}

static void on_regen(void)         // SW10 – momentary; sent as bit on 0x701
{
    (void)0;
}

// Debounce: only trigger handlers on rising edge
static bool prev_pressed[NUM_ROWS][NUM_COLUMNS] = {{false}};

// [row][col]. NULL = no button
static const button_handler_t button_handlers[NUM_ROWS][NUM_COLUMNS] = {
    { on_left_light,  on_right_light, on_hazards,  on_brights   },  /* row 0 */
    { on_cruise_en,   on_cruise_up,   on_cruise_down, NULL       },  /* row 1 */
    { on_horn,        on_ptt,        on_regen,    NULL         },  /* row 2 */
};

int main()
{
    pico_canlib g_can = pico_canlib();
    pico_canlib::status errorCode;
    errorCode = g_can.init();
    fprintf(stdout, "Init Code %d\n", errorCode);

    if (errorCode != pico_canlib::status::SUCCESS)
    {
        fprintf(stdout, "Failed Startup\n");
    }

    for (int r = 0; r < NUM_ROWS; r++) {
        gpio_init(row_pins[r]);
        gpio_set_dir(row_pins[r], GPIO_IN);
        gpio_pull_down(row_pins[r]);
    }
    for (int c = 0; c < NUM_COLUMNS; c++) {
        gpio_init(col_pins[c]);
        gpio_set_dir(col_pins[c], GPIO_OUT);
        gpio_put(col_pins[c], 0);
    }
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

#if (BRAKE_INPUT_PIN != BRAKE_PIN_DISABLED)
    gpio_init(BRAKE_INPUT_PIN);
    gpio_set_dir(BRAKE_INPUT_PIN, GPIO_IN);
    gpio_pull_down(BRAKE_INPUT_PIN);
#endif

    while (true) {
        bool button_pressed[NUM_ROWS][NUM_COLUMNS] = {{false}};

        for (int col = 0; col < NUM_COLUMNS; col++) {
            for (int c = 0; c < NUM_COLUMNS; c++) {
                gpio_put(col_pins[c], (c == col));
            }
            sleep_ms(SCAN_DELAY_MS);

            for (int row = 0; row < NUM_ROWS; row++) {
                bool now = gpio_get(row_pins[row]) != 0;
                button_pressed[row][col] = now;
                if (now && !prev_pressed[row][col]) {
                    button_handler_t handler = button_handlers[row][col];
                    if (handler != NULL) {
                        handler();
                    }
                }
                prev_pressed[row][col] = now;
            }
        }

        bool brake = false;
#if (BRAKE_INPUT_PIN != BRAKE_PIN_DISABLED)
        brake = gpio_get(BRAKE_INPUT_PIN) != 0;
#endif

        send_steering_wheel_can_state(
            button_pressed[1][1],
            button_pressed[1][2],
            button_pressed[2][0],
            button_pressed[2][2],
            brake
        );

        // Process received CAN (Power Distro → Steering Wheel 0x801).
        // Ensure xl2515 RX filter accepts 0x801 
        {
            uint32_t id;
            uint8_t rx_data[8];
            uint8_t rx_len = 0;
            if (!(int)g_can.receiveCAN(&id, rx_data, 4, 8) && id == POWER_DISTRO_TO_STEERING_WHEEL_ID) {
                process_power_distro_status(rx_data, rx_len);
            }
            //if (xl2515_recv(&id, rx_data, &rx_len) && id == POWER_DISTRO_TO_STEERING_WHEEL_ID) {
                //process_power_distro_status(rx_data, rx_len);
            //}
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
        gpio_put(LED_PIN, led_on ? 1 : 0);
    }
}
