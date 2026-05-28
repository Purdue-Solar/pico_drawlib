#include "main.hpp"
#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include "ili9341.h"
#include "multicore.hpp"
#include "pico_canlib.hpp"
#include "artemis_canid.hpp"

PDLInfo sharedInfo = {0};
mutex_t info_mutex;

int main()
{
    stdio_init_all();
    mutex_init(&info_mutex);
    pico_canlib can = pico_canlib();
    pico_canlib::status errorCode;
    errorCode = can.init();
    fprintf(stdout, "Init Code %d\n", errorCode);

    if (errorCode != pico_canlib::status::SUCCESS)
    {
        fprintf(stdout, "Failed Startup\n");
    }

    Accelerator accelerator;
    accelerator.init();

    if constexpr (FEAT_BRAKE_PIN) 
    {
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

    uint8_t buffer[13];
    uint32_t id;
    uint8_t data[8];

    multicore_launch_core1(core1_entry);

    while(true)
    {
        mutex_enter_blocking(&info_mutex);
        // Read momentary buttons directly from the matrix (held state)
        wheel.horn        = matrix.button_pressed[2][0];
        wheel.cruise_up   = matrix.button_pressed[1][1];
        wheel.cruise_down = matrix.button_pressed[1][2];
        wheel.regen       = matrix.button_pressed[2][2];
        if constexpr (FEAT_BRAKE_PIN) 
        {
            wheel.brake = gpio_get(brakeInputPin) != 0;
        }

        send_steering_wheel_can_state(wheel, &can);

        // Process received CAN (Power Distro → Steering Wheel artemis_canid::powerDistroToSteering).
        {
            // Buffer layout per new receiveCAN API: [0–3] id, [4] data length, [5–12] data
            uint8_t rx_buf[13];
            if (!(int)can.receiveCAN(rx_buf, 4, 8))
            {
                uint32_t id;
                memcpy(&id, rx_buf, 4);
                uint8_t * data = &rx_buf[5];
                switch (id)
                {
                case canIDHelper(artemis_canid::powerDistroToSteering):
                    // Use rx_buf[4] to pass data length
                    process_power_distro_status(data, rx_buf[4], &sharedInfo);
                    break;
                case canIDHelper(artemis_canid::tempAndSOC):
                    sharedInfo.battery_temperature = data[7];
                    sharedInfo.battery_soc = data[4];
                    break;
                case canIDHelper(artemis_canid::setMotorCurrent):
                    sharedInfo.motor_current = data[7] << 24 | data[6] << 16 | data[5] << 8 | data[4];
                    break;
                case canIDHelper(artemis_canid::motorVelocity):
                    sharedInfo.motor_velocity = data[7] << 24 | data[6] << 16 | data[5] << 8 | data[4];
                    break;
                }
            }
        }

        accelerator.update(can);
        //sharedInfo.motor_velocity++;

        mutex_exit(&info_mutex);
        }
}