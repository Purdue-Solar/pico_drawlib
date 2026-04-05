#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <iostream>
#include <string>

/// @brief XL2515 Commands
class XL2515
{
public:
    static const uint XL2515_BAUDRATE = 10000000;

    static const uint8_t NORMAL_MODE = 0x00;
    static const uint8_t LOOPBACK_MODE = 0x40;
    static const uint8_t INTE_EN = 0x03; // 0x03 for rxb0 and rxb1 interrupt enable
    static const uint8_t NORMAL_CNF1 = 0x00;
    static const uint8_t NORMAL_CNF2 = 0x9E;
    static const uint8_t NORMAL_CNF3 = 0x03;

    enum class PRIORITY : uint8_t
    {
        Lowest = 0,
        Low,
        High,
        Highest
    };

    enum class IN_ADDR : uint8_t
    {
        CANINTE = 0x2B,
        CANCTRL = 0x0F,
        CNF1 = 0x2A,
        CNF2 = 0x29,
        CNF3 = 0x28,
        TXBxCTRL = 0x30,
        TXBxSIDH = 0x31,
        TXBxSIDL = 0x32,
        TXBxEID8 = 0x33,
        TXBxEID0 = 0x34,
        TXBxDLC = 0x35,
        CANINTF = 0x2C, // interrupt flag register (added for clearing RX flags)
    };

    /// @brief XL2515 SPI Communication Protocol
    enum class SPI_INSTR_XL : uint8_t
    {
        RESET = 0xC0,
        READ = 0x03,
        READ_RX_BUFF = 0x90,
        WRITE = 0x02,
        LOAD_TX_BUFF = 0x40, // might need to change to 0x41, 0x41 sends to data buffer, not command buffer
        RTS = 0x80,
        READ_STATUS = 0xA0,
        RX_STATUS = 0xB0,
        BIT_MODIFY = 0x05,
        REQTS = 0x80
    };

    /// @brief Mask for READ_RX_BUFF
    enum class dir_RX_Address : uint8_t
    {
        dir_RXB0_ID,
        dir_RXB1_ID,
        dir_RXB0_DAT,
        dir_RXB1_DAT
    };

    /// @brief Mask for LOAD_TX_BUFF
    enum class dir_TX_Address : uint8_t
    {
        dir_TXB0_ID,
        dir_TXB1_ID,
        dir_TXB2_ID,
        dir_TXB0_DAT,
        dir_TXB1_DAT,
        dir_TXB2_DAT
    };

    /// @brief TX_Buffer Selection Mask
    enum class TX_BUFFER_SEL : uint8_t
    {
        TX0 = 0,
        TX1 = 1,
        TX2 = 2,
    };

    /// @brief RX_Buffer Selection Mask
    enum class RX_BUFFER_SEL : uint8_t
    {
        RX0 = 0,
        RX1 = 1,
    };

    static constexpr uint8_t write_buffer_len = 14;
    /// @brief WRITE SPI payload struct
    struct load_tx_buffer
    {
        uint8_t instr;
        uint8_t *payload; // Length 8 bytes
    };

    /// @brief READ SPI payload struct
    struct write_buffer
    {
        uint8_t instruction;
        uint8_t addr;
        uint8_t payload;
    };

    struct bit_modify
    {
        uint8_t instruction = (uint8_t)SPI_INSTR_XL::BIT_MODIFY;
        uint8_t addr;
        uint8_t mask;
        uint8_t payload;
    };

    /// @brief READ_RX_BUFFER SPI payload struct
    struct dir_read_rx_buffer
    {
        uint8_t instruction;
    };

private:
};

/// @brief RP2350 functions to communicate with XL2515 CAN tranciever
class pico_canlib
{
public:
    /// @brief Class constructor
    /// @param gpioInt GPIO Interrupt
    /// @param miso Master In Slave Out (MISO) pin
    /// @param mosi Master Out Slave In (MOSI) pin
    /// @param cs   Chip Select Pin
    /// @param sck  Serial CLK pin
    /// @param spi_hw SPI peripheral address
    pico_canlib(uint8_t gpioInt = 8, uint8_t miso = 12, uint8_t mosi = 11, uint8_t cs = 9, uint8_t sck = 10, spi_inst_t *spi_hw = spi1) : in_intGPIO(gpioInt), in_miso(miso), in_mosi(mosi), in_cs(cs), in_sck(sck), in_spi_hw(spi_hw) {};
    // ~pico_canlib();

    /// @brief Error Codes when using CAN, only use these if CAN is used
    enum class status : uint16_t
    {
        SUCCESS = 0,
        TX_ID_COMMAND_ERROR = 1,
        TX_PAYLOAD_COMMAND_ERROR = 2,
        TX_ID_ERROR = 3,
        TX_PAYLOAD_ERROR = 4,
        RX_ID_ERROR = 5,
        RX_PAYLOAD_ERROR = 6,
        STATUS_ERROR = 7,
        STATUS_DONE = 8,
        STATUS_STALL = 9,
        TX_STATUS_ERROR = 10,
        TX_STATUS_DONE = 11,
        TX_STATUS_STALL = 12,
        RESET_ERROR = 13,
        RESET_INIT_ERROR = 14,
        REQUESTTS_ERROR = 15,
        MODIFIED_ERROR = 16,
        GET_CONTROL_BITS_ERROR = 17,
        GET_CONTROL_BITS_INSTR_ERROR = 18,
        SET_CONTROL_BITS_ERROR = 19,
        WRITE_ERROR = 20,
        INIT_ERROR = 21,
        RX_STATUS_ERROR = 22, // failure reading RX status register
        NO_NEW_MESSAGE = 23
    }; // Don't ask me why I label them even though this is an enum class. It is for ease of debugging status

    /// @brief Initialized spi ports and reset XL2515 Configuration
    status init();

    /// @brief Send SPI request to send CAN messages
    /// @param TX_SEL TX Buffer select (0, 1, or 2)
    /// @param id    29 bits extended ids as bytes array
    /// @param TX_buffer  Payload bytes array
    /// @param length Length in bytes of payload
    /// @return True if SPI request was successful sent
    status transmitCAN(XL2515::TX_BUFFER_SEL TX_SEL, uint32_t can_id, bool isExtended, uint8_t *data_buffer, uint8_t data_length, XL2515::PRIORITY priority);

    /// @brief Send SPI request to recieve CAN messages
    /// @param buffer 0-3 are id, 4 is data length, 5-12 are data
    /// @param idSize Length in bytes of id
    /// @param bufferSize Length in bytes of payload
    /// @return True if SPI request was successful sent
    status receiveCAN(uint8_t *buffer, uint8_t idSize, uint8_t bufferSize);

    /// @brief Check can status register (Tell setting of the tranciever)
    /// @param status Return status byte (XL2515 Specifics. Check datasheet)
    /// @return Status of function
    status checkStatus(uint8_t *status);

    /// @brief
    /// @param bytes
    /// @return
    status getByte(uint8_t *bytes, XL2515::IN_ADDR addr);

    /// @brief
    /// @param bytes
    /// @param addr
    /// @return
    status setByte(uint8_t bytes, XL2515::IN_ADDR addr);

    /// @brief
    /// @param addr
    /// @return
    uint8_t getBit(uint8_t addr);

    /// @brief sets filters and masks, don't need to use
    /// @param length
    /// @param addr
    /// @return
    status filtersAndMasks(int length, XL2515::IN_ADDR addr);

private:
    /// Idk LED indicator or smth, default LED pin is 25 on Pico 2. Thinking of implementing it but it won't fix my problem just excessive. Cool feature to have tho
    bool isLED;
    uint in_LEDPin;
    uint8_t in_intGPIO;
    uint8_t in_miso;
    uint8_t in_mosi;
    uint8_t in_cs;
    uint8_t in_sck;
    spi_inst_t *in_spi_hw;

    /// @brief Device Request To Send Signal
    /// @param buffer Determine which TX buffer to use
    status requestTS(uint8_t buffer);

    /// @brief Reset XL2515 Configuration
    status reset();

    status modifiedBit(uint8_t bytes, uint8_t address, uint8_t masked);

    // status setControlBits(uint8_t bytes);
};