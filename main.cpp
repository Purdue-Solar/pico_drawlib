#include "main.hpp"

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
