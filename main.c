#include <stdio.h>
#include "pico/stdlib.h"
#include "xl2515.h"

#define LED_PIN 25
#define R0 16
#define R1 17
#define R2 13
#define C0 15
#define C1 14
#define C2 18
#define C3 19

#define NUM_ROWS    3
#define NUM_COLUMNS 4
#define SCAN_DELAY_MS 10
#define LED_PIN 25

// Row pins (inputs)
static const uint row_pins[NUM_ROWS] = { 16, 17, 13 };

// Column pins (outputs)
static const uint col_pins[NUM_COLUMNS] = { 15, 14, 18, 19 };

// Function called when a button is pressed. NULL = no button
typedef void (*button_handler_t)(void);

// Add logic
static void on_left_light(void)    { /* SW01 */ }
static void on_right_light(void)   { /* SW02 */ }
static void on_hazards(void)       { /* SW03 */ }
static void on_brights(void)       { /* SW04 */ }
static void on_cruise_en(void)     { /* SW05 */ }
static void on_cruise_up(void)     { /* SW06 */ }
static void on_cruise_down(void)   { /* SW07 */ }
static void on_horn(void)          { /* SW08 */ }
static void on_ptt(void)           { /* SW09 */ }
static void on_regen(void)         { /* SW10 */ }

// [row][col]. NULL = no button
static const button_handler_t button_handlers[NUM_ROWS][NUM_COLUMNS] = {
    { on_left_light,  on_right_light, on_hazards,  on_brights   },  /* row 0 */
    { on_cruise_en,   on_cruise_up,   on_cruise_down, NULL       },  /* row 1 */
    { on_horn,        on_ptt,        on_regen,    NULL         },  /* row 2 */
};

int main()
{
    stdio_init_all();

    for (int r = 0; r < NUM_ROWS; r++) {
        gpio_init(row_pins[r]);
        gpio_set_dir(row_pins[r], GPIO_IN);
    }
    for (int c = 0; c < NUM_COLUMNS; c++) {
        gpio_init(col_pins[c]);
        gpio_set_dir(col_pins[c], GPIO_OUT);
    }
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        for (int col = 0; col < NUM_COLUMNS; col++) {
            // Drive one column high, others low
            for (int c = 0; c < NUM_COLUMNS; c++) {
                gpio_put(col_pins[c], (c == col));
            }
            sleep_ms(SCAN_DELAY_MS);

            // Read all rows, call handler if button present
            for (int row = 0; row < NUM_ROWS; row++) {
                if (gpio_get(row_pins[row])) {
                    button_handler_t handler = button_handlers[row][col];
                    if (handler != NULL) {
                        handler();
                    }
                }
            }
        }
    }
}
