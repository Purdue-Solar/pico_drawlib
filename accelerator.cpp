#include "accelerator.hpp"
#include "pico_canlib.hpp"
#include <stdlib.h>
#include <string.h>

static float g_pedal_value = 0.0f; // 0.0 to 1.0
static uint8_t g_drive_state = DRIVE_NEUTRAL;

// read FNR pins and update drive state (g_drive_state)
static void update_drive_state(void)
{
    bool forward = gpio_get(FNR_FORWARD_PIN);
    bool reverse = gpio_get(FNR_REVERSE_PIN);

    if(forward){
        g_drive_state = DRIVE_FORWARD;
    }
    else if(reverse){
        g_drive_state = DRIVE_REVERSE;
    }
    else{
        g_drive_state = DRIVE_NEUTRAL;
    }
}

// read ADC from accelerator pedal, applies deadzone, and update g_pedal_value 
static void update_pedal_value(void)
{
    uint16_t raw_pedal = adc_read();

    // convert to 0.0 - 1.0 to send to motor (can change if necessary)
    float converted_pedal = raw_pedal / 4095.0f;

    if (converted_pedal < PEDAL_DEADZONE){
        converted_pedal = 0.0f;
    }
    g_pedal_value = converted_pedal;
}

//initializing pins
void accelerator_init(void)
{
    gpio_init(FNR_FORWARD_PIN);
    gpio_set_dir(FNR_FORWARD_PIN, GPIO_IN);
    gpio_pull_down(FNR_FORWARD_PIN);
 
    gpio_init(FNR_REVERSE_PIN);
    gpio_set_dir(FNR_REVERSE_PIN, GPIO_IN);
    gpio_pull_down(FNR_REVERSE_PIN);
 
    // ADC pin for accelerator
    adc_init(); 
    adc_gpio_init(ACCELERATOR_PIN);
    adc_select_input(0); // uses ADC0 channel 0 (GPIO26)
}

//sending CAN message to motor (FEEL FREEE TO CHANGE IM NOT SURE IF THIS IS HOW TO SEND CURRENT AND VELOCITY INFO THRU CAN PROPERLY)
void send_motor_command(pico_canlib& can)
{
    float current = 0.0f;
    float max_velocity = 0.0f;

    if (g_drive_state == DRIVE_FORWARD) {
        current  = g_pedal_value;
        max_velocity = MOTOR_POSITIVE_RPM;
    }
    else if (g_drive_state == DRIVE_REVERSE) {
        current  = g_pedal_value * 0.5f; //limit reverse to 50% power for safety
        max_velocity = MOTOR_NEGATIVE_RPM;
    }
    else {
        current = 0.0f;
        max_velocity = 0.0f;
    }

    uint8_t current_data[4];
    memcpy(current_data, &current, 4);
    can.transmitCAN(XL2515::TX_BUFFER_SEL::TX0, MOTOR_CURRENT_CAN_ID, false, current_data, 4, XL2515::PRIORITY::Highest);

    uint8_t velocity_data[4];
    memcpy(velocity_data, &max_velocity, 4);
    can.transmitCAN(XL2515::TX_BUFFER_SEL::TX1, MOTOR_VELOCITY_CAN_ID, false, velocity_data, 4, XL2515::PRIORITY::Highest);
}

//call in main to get pin inputs and update accelerator
void accelerator_update(pico_canlib& can)
{
    update_drive_state();
    update_pedal_value();
    send_motor_command(can);
}