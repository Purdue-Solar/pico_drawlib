#include "main.hpp"
#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include "ili9341.h"

#ifdef SIMULATION
#include "picosdk_sim.h"
#else

extern "C"
{
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "pdl.h"
#include "gfx.h"
}

#include "pico_canlib.hpp"
#include "artemis_canid.hpp"
#endif

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
    // Should fix so nums are configured in features.hpp
    Matrix matrix(0, 1, 5, rowPins, NumRows, colPins, NumCols, {{
        { on_hazards, on_right_light, on_horn },  /* row 0 */
        { on_brights,  on_cruise_up, on_cruise_down },  /* row 1 */
        { on_cruise_en, on_left_light, on_regen, },  /* row 2 */
    }});
    
    matrix.matrix_init();
    matrix.keypad_init_timer();

    PDLInfo info = {0};
    uint8_t buffer[13];
    uint32_t id;
    uint8_t data[8];

    stdio_init_all();
    pico_canlib can = pico_canlib();
    can.init();

    bool led_on = true;

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    // gpio_put(PICO_DEFAULT_LED_PIN, led_on);

    gpio_init(22);
    gpio_set_dir(22, GPIO_OUT);
    gpio_put(22, led_on);

    LCD_setPins(20, 17, 21, 18, 19);
    LCD_setSPIperiph(spi0);
    LCD_initDisplay();
    LCD_setRotation(1);
    GFX_createFramebuf();

#ifdef SIMULATION
    LCDSim_InitWindow();
    while (!LCDSim_WindowShouldClose())
#else
    while (true) 
#endif
    {
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
                uint8_t * data = &rx_buf[5];
                //id = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
                if (id == canIDHelper(artemis_canid::powerDistroToSteering)) {
                    // Go to rx_buf + 5 to start at data, use rx_buf[4] to pass data length
                    process_power_distro_status(rx_buf + 5, rx_buf[4]);
                }
                else if (id == canIDHelper(artemis_canid::tempAndSOC))
                {
                    gpio_put(PICO_DEFAULT_LED_PIN, true);
                }
                switch (id)
                {
                case canIDHelper(artemis_canid::tempAndSOC):
                    info.battery_temperature = data[7];
                    info.battery_soc = data[4];
                    break;
                case canIDHelper(artemis_canid::setMotorCurrent):
                    info.motor_current = data[7] << 24 | data[6] << 16 | data[5] << 8 | data[4];
                    break;
                case canIDHelper(artemis_canid::motorVelocity):
                    info.motor_velocity = data[7] << 24 | data[6] << 16 | data[5] << 8 | data[4];
                    break;
                case canIDHelper(artemis_canid::steeringWheel):
                    info.main_valid = data[0] & 0b00000001;
                    info.aux_valid = data[0] & 0b00000010;
                    info.main_current_warning = data[0] & 0b00010000;
                    info.main_over_current = data[0] & 0b00100000;
                    info.main_under_voltage = data[0] & 0b01000000;
                    info.main_over_voltage = data[0] & 0b10000000;

                    info.aux_current_warning = data[1] & 0b00000001;
                    info.aux_over_current = data[1] & 0b00000010;
                    info.aux_under_voltage = data[1] & 0b00000100;
                    info.aux_over_voltage = data[1] & 0b00001000;

                    break;
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
        info.motor_velocity++;

        pdl_draw(&info);
        #ifdef SIMULATION
        LCDSim_Redraw();
        #endif
        }
#ifdef SIMULATION
    LCDSim_CloseWindow();
#endif
}