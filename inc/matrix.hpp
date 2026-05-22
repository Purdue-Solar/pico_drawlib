#pragma once
#include <array>
#include <cstdint>

class Matrix
{
public:
    static constexpr uint8_t MAX_ROWS = 8;
    static constexpr uint8_t MAX_COLS = 8;

    typedef void (*button_handler_t)(void);
    // Indexed [row][col] — outer dimension is row, inner is col
    using HandlerTable = std::array<std::array<button_handler_t, MAX_COLS>, MAX_ROWS>;

    Matrix(
        uint8_t driveColumnTimerNumC,
        uint8_t isrTimerNumC,
        uint8_t scanDelayMSC,
        const uint8_t* rowPinsC,
        uint8_t numRowsC,
        const uint8_t* colPinsC,
        uint8_t numColsC,
        const HandlerTable& handlersC = {}
    )
        :
        driveColumnTimerNum(driveColumnTimerNumC),
        isrTimerNum(isrTimerNumC),
        scanDelayMS(scanDelayMSC),
        rowPins(rowPinsC),
        numRows(numRowsC),
        colPins(colPinsC),
        numCols(numColsC),
        button_handlers(handlersC)
    {
        make_colmask();
    }

    // Indexed [row][col]
    volatile bool button_pressed[MAX_ROWS][MAX_COLS] {};
    HandlerTable button_handlers {};

    void matrix_init(void);
    void keypad_init_timer(void);
    uint8_t keypad_read_rows();
    static void keypad_drive_column(void);
    static void keypad_isr(void);

private:
    static Matrix* s_instance;

    void make_colmask()
    {
        colMask = 0;

        for(uint8_t i = 0; i < numCols; i++)
        {
            uint8_t pin = colPins[i];

            colMask |= 1u << pin;
        }
    }

    uint64_t colMask;
    uint8_t driveColumnTimerNum;
    uint8_t isrTimerNum;
    uint8_t scanDelayMS;

    uint8_t numCols;
    uint8_t numRows;
    const uint8_t* rowPins;
    const uint8_t* colPins;
    int16_t currentCol = -1;
};