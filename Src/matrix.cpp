#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <hardware/irq.h>
#include <stdio.h>
#include "features.hpp"
#include "pico/stdlib.h"
#include <algorithm>
#include "matrix.hpp"

Matrix* Matrix::s_instance = nullptr;

void Matrix::matrix_init (void) {
   for (int r = 0; r < numRows; r++) {
        gpio_init(rowPins[r]);
        gpio_set_dir(rowPins[r], GPIO_IN);
        gpio_pull_down(rowPins[r]);
    }
    for (int c = 0; c < numCols; c++) {
        gpio_init(colPins[c]);
        gpio_set_dir(colPins[c], GPIO_OUT);
        gpio_put(colPins[c], 0);
    }
}

void Matrix::keypad_init_timer() {
    s_instance = this;
    // See rp2350 datasheet, page 1185
    hw_set_bits(&timer_hw->inte, 1u << driveColumnTimerNum);
    hw_set_bits(&timer_hw->inte, 1u << isrTimerNum);
    uint alarm_irq = timer_hardware_alarm_get_irq_num(timer_hw, driveColumnTimerNum);
    uint alarm_irq2 = timer_hardware_alarm_get_irq_num(timer_hw, isrTimerNum);
    irq_set_exclusive_handler(alarm_irq, keypad_drive_column);
    irq_set_exclusive_handler(alarm_irq2, keypad_isr);
    irq_set_enabled(alarm_irq, true);
    irq_set_enabled(alarm_irq2, true);
    uint64_t drive_column = timer_hw-> timerawl + 100000;
    uint64_t isr = timer_hw->timerawl + 100000 + 1000 * scanDelayMS;
    timer_hw->alarm[driveColumnTimerNum] = (uint32_t) drive_column;
    timer_hw->alarm[isrTimerNum] = (uint32_t) isr;
}

void Matrix::keypad_drive_column() {
    Matrix* m = s_instance;
    hw_clear_bits(&timer_hw->intr, 1u << m->driveColumnTimerNum);
    int col_prev = m->currentCol;
    m->currentCol = (m->currentCol + 1) % m->numCols;
    uint64_t toggleMask = (1u << (m->colPins[m->currentCol]) | (1u << (m->colPins[col_prev]) && col_prev != -1));
    gpioc_hilo_out_xor(m->colMask | toggleMask);
    timer_hw->alarm[m->driveColumnTimerNum] = timer_hw->timerawl + 25000;
}

uint8_t Matrix::keypad_read_rows() {

    if (numRows > 8)
    {
        //Throw error
        return 0;
    }

    uint8_t rowData = 0;
    for(int i = 0; i < numRows; i++)
    {
        rowData |= gpioc_hilo_in_get();
    }

    return rowData;
}

void Matrix::keypad_isr() {
    Matrix* m = s_instance;
    hw_clear_bits(&timer_hw->intr, 1u << m->isrTimerNum);
    uint8_t rows = m->keypad_read_rows();
    for (int i = 0; i < m->numRows; i++)
    {
        bool now = (rows >> i) & 1u;
        volatile bool& state = m->button_pressed[i][m->currentCol];
        if (now && !state)
        {
            state = true;
            if (m->button_handlers[i][m->currentCol])
            {
                m->button_handlers[i][m->currentCol]();
            }
        }
        else if (!now && state)
        {
            state = false;
        }
    }
    timer_hw->alarm[m->isrTimerNum] = timer_hw->timerawl + 25000;
}
