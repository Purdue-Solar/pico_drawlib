#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "xl2515.h"

// Input pins
#define FNR_FORWARD_PIN 6
#define FNR_REVERSE_PIN 7
#define ACCELERATOR_PIN 26  // GPIO26_ADC0

// Drive states
#define DRIVE_NEUTRAL 0
#define DRIVE_FORWARD 1
#define DRIVE_REVERSE 2

#define PEDAL_DEADZONE 0.05f // 5% deadzone (can change if necessary)

// Motor CAN IDs
#define MOTOR_CURRENT_CAN_ID 0x501 
#define MOTOR_VELOCITY_CAN_ID 0x502

//Motor velocity constants
#define MOTOR_POSITIVE_RPM 20000.0f  
#define MOTOR_NEGATIVE_RPM -20000.0f

//intiializing pins and updating acceklerator
void accelerator_init(void);
void accelerator_update(void);


















