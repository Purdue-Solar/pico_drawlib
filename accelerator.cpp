#include "accelerator.hpp"
#include "pico_canlib.hpp"
#include "artemis_canid.hpp"
#include "features.hpp"
#include <stdlib.h>
#include <string.h>

// read FNR pins and update drive state (m_drive_state)
void Accelerator::update_drive_state(void)
{
    bool forward = gpio_get(FNRForwardPin);
    bool reverse = gpio_get(FNRReversePin);

    if(forward){
        m_drive_state = DriveDirection::Forward;
    }
    else if(reverse){
        m_drive_state = DriveDirection::Reverse;
    }
    else{
        m_drive_state = DriveDirection::Neutral;
    }
}

// read ADC from accelerator pedal, applies deadzone, and update m_pedal_value 
void Accelerator::update_pedal_value(void)
{
    uint16_t raw_pedal = adc_read();

    // convert to 0.0 - 1.0 to send to motor (can change if necessary)
    float converted_pedal = raw_pedal / 4095.0f;

    if (converted_pedal < pedalDeadzone){
        converted_pedal = 0.0f;
    }
    m_pedal_value = converted_pedal;
}

//initializing pins
void Accelerator::init(void)
{
    gpio_init(FNRForwardPin);
    gpio_set_dir(FNRForwardPin, GPIO_IN);
    gpio_pull_down(FNRForwardPin);
 
    gpio_init(FNRReversePin);
    gpio_set_dir(FNRReversePin, GPIO_IN);
    gpio_pull_down(FNRReversePin);
 
    // ADC pin for accelerator
    adc_init(); 
    adc_gpio_init(AcceleratorPin);
    adc_select_input(0); // uses ADC0 channel 0 (GPIO26)
}

//sending CAN message to motor (FEEL FREEE TO CHANGE IM NOT SURE IF THIS IS HOW TO SEND CURRENT AND VELOCITY INFO THRU CAN PROPERLY)
void Accelerator::send_motor_command(pico_canlib& can)
{
    float current = 0.0f;
    float max_velocity = 0.0f;

    if (m_drive_state == DriveDirection::Forward) {
        current  = m_pedal_value;
        max_velocity = motorPositiveRPM;
    }
    else if (m_drive_state == DriveDirection::Reverse) {
        current  = m_pedal_value * 0.5f; //limit reverse to 50% power for safety
        max_velocity = motorNegativeRPM;
    }
    else {
        current = 0.0f;
        max_velocity = 0.0f;
    }

    uint8_t current_data[4];
    memcpy(current_data, &current, 4);
    can.transmitCAN(XL2515::TX_BUFFER_SEL::TX0, canIDHelper(artemis_canid::motorCurrentCanID), false, current_data, 4, XL2515::PRIORITY::Highest);

    uint8_t velocity_data[4];
    memcpy(velocity_data, &max_velocity, 4);
    can.transmitCAN(XL2515::TX_BUFFER_SEL::TX1, canIDHelper(artemis_canid::motorVelocityCanID), false, velocity_data, 4, XL2515::PRIORITY::Highest);
}

//call in main to get pin inputs and update accelerator
void Accelerator::update(pico_canlib &can)
{
    update_drive_state();
    update_pedal_value();
    send_motor_command(can);
}