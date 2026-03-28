#include <stdint.h>

#include "ili9341.h"

#ifdef SIMULATION
#include "picosdk_sim.h"
#else
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "pico_canlib.hpp"
#include "artemis_canid.hpp"
#include "pdl.h"
#endif

#include <stdio.h>
#include <cstring>

#define CAN_ID 0x

int main()
{
    PDLInfo info;
    uint8_t buffer[13];
    uint32_t id;
    uint8_t data[8];

    stdio_init_all();
    pico_canlib can = pico_canlib();
    can.init();

    bool led_on = true;

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);

    gpio_init(22);
    gpio_set_dir(22, GPIO_OUT);
    gpio_put(22, led_on);

    LCD_setPins(20, 17, 21, 18, 19);
    LCD_setSPIperiph(spi0);
    LCD_initDisplay();
    LCD_setRotation(1);

#ifdef SIMULATION
    LCDSim_InitWindow();

    while (!LCDSim_WindowShouldClose())
#else
    while (true)
#endif
    {
        if (can.receiveCAN(buffer, 4, 8) == pico_canlib::status::SUCCESS)
        {
            id = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];

            for (int i = 0; i < 8; i++)
            {
                data[i] = buffer[5 + i];
            }

            switch (id)
            {
            case artemis_canid::tempAndSOC:
                info.battery_temperature = data[7];
                info.battery_soc = data[4];
                break;
            case artemis_canid::setMotorCurrent:
                info.motor_current = data[7] << 24 | data[6] << 16 | data[5] << 8 | data[4];
                break;
            case artemis_canid::motorVelocity:
                info.motor_velocity = data[7] << 24 | data[6] << 16 | data[5] << 8 | data[4];
                break;
            case artemis_canid::steeringWheel:
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
#ifdef SIMULATION
        LCDSim_Redraw();
#endif
    }

#ifdef SIMULATION
    LCDSim_CloseWindow();
#endif
}