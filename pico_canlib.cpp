#include "pico_canlib.hpp"

pico_canlib::status pico_canlib::init(void)
{
    // Initialize SPI port at 1 MHz
    spi_init(in_spi_hw, XL2515::XL2515_BAUDRATE);

    // Set the GPIO functions for the SPI pins
    gpio_set_function(in_miso, GPIO_FUNC_SPI);
    gpio_set_function(in_mosi, GPIO_FUNC_SPI);
    gpio_set_function(in_cs, GPIO_FUNC_SIO); // CS pin as SIO for manual control
    gpio_set_function(in_sck, GPIO_FUNC_SPI);

    // Set CS pin high (inactive)
    gpio_init(in_cs);
    gpio_set_dir(in_cs, GPIO_OUT);
    gpio_put(in_cs, 1);

    // Reset XL2515 Configuration
    status status = reset();
    fprintf(stdout, "Status Bytes = %d\n", status);
    if (status != status::SUCCESS)
    {
        return status;
    }

    // // set rx to recieve all messages (no filtering) by setting masks to 0 and filters to 0
    // filtersAndMasks(14, (XL2515::IN_ADDR)0x00);
    // filtersAndMasks(14, (XL2515::IN_ADDR)0x10);
    // filtersAndMasks(10, (XL2515::IN_ADDR)0x20);
    // // set rxb0ctrl to receive all messages (no filtering)
    // setByte(0x64, (XL2515::IN_ADDR)0x60); // receive all valid messages with standard or extended identifiers
    // setByte(0x60, (XL2515::IN_ADDR)0x70);

    // Set Control Bits
    uint8_t mode;

    fprintf(stdout, "CNF1 SET. Error Status: %d\n", setByte(XL2515::NORMAL_CNF1, XL2515::IN_ADDR::CNF1));
    getByte(&mode, XL2515::IN_ADDR::CNF1);
    fprintf(stdout, "CANINTE Bytes = %d\n", mode);

    if (mode != XL2515::NORMAL_CNF1)
    {
        return status::INIT_ERROR;
    }

    fprintf(stdout, "CNF1 SET. Error Status: %d\n", setByte(XL2515::NORMAL_CNF2, XL2515::IN_ADDR::CNF2));
    getByte(&mode, XL2515::IN_ADDR::CNF2);
    fprintf(stdout, "CANINTE Bytes = %d\n", mode);

    if (mode != XL2515::NORMAL_CNF2)
    {
        return status::INIT_ERROR;
    }

    fprintf(stdout, "CNF1 SET. Error Status: %d\n", setByte(XL2515::NORMAL_CNF3, XL2515::IN_ADDR::CNF3));
    getByte(&mode, XL2515::IN_ADDR::CNF3);
    fprintf(stdout, "CANINTE Bytes = %d\n", mode);

    if (mode != XL2515::NORMAL_CNF3)
    {
        return status::INIT_ERROR;
    }

    fprintf(stdout, "CANINTE SET. Error Status: %d\n", setByte(XL2515::INTE_EN, XL2515::IN_ADDR::CANINTE));
    getByte(&mode, XL2515::IN_ADDR::CANINTE);
    fprintf(stdout, "CANINTE Bytes = %d\n", mode);

    if (mode != XL2515::INTE_EN)
    {
        return status::INIT_ERROR;
    }

    fprintf(stdout, "CANCTRL SET. Error Status: %d\n", setByte(XL2515::NORMAL_MODE, XL2515::IN_ADDR::CANCTRL));
    getByte(&mode, XL2515::IN_ADDR::CANCTRL);
    fprintf(stdout, "CANCONTROL Bytes = %d\n", mode);

    if (mode == XL2515::LOOPBACK_MODE)
    {
        return status::INIT_ERROR;
    }

    return status::SUCCESS;
}

pico_canlib::status pico_canlib::filtersAndMasks(int length, XL2515::IN_ADDR addr)
{
    uint8_t message[length] = {0};
    message[0] = (uint8_t)XL2515::SPI_INSTR_XL::WRITE;
    message[1] = (uint8_t)addr;

    gpio_put(in_cs, 0);
    if (spi_write_blocking(in_spi_hw, message, 14) != length)
    {
        gpio_put(in_cs, 1);
        return status::WRITE_ERROR;
    }
    gpio_put(in_cs, 1);
    return status::SUCCESS;
}

pico_canlib::status pico_canlib::setByte(uint8_t bytes, XL2515::IN_ADDR addr)
{
    uint8_t message[4];
    message[0] = (uint8_t)XL2515::SPI_INSTR_XL::WRITE;
    message[1] = (uint8_t)addr;
    // message[2] = 0xFF;
    message[2] = bytes;

    gpio_put(in_cs, 0);

    if (spi_write_blocking(in_spi_hw, message, 3) != 3)
    {
        gpio_put(in_cs, 1);
        return status::WRITE_ERROR;
    }

    gpio_put(in_cs, 1);

    return status::SUCCESS;
}

pico_canlib::status pico_canlib::getByte(uint8_t *bytes, XL2515::IN_ADDR addr)
{
    uint8_t data[3];
    data[0] = (uint8_t)XL2515::SPI_INSTR_XL::READ;
    data[1] = (uint8_t)addr;

    gpio_put(in_cs, 0);

    spi_write_blocking(in_spi_hw, data, 2);
    if (spi_read_blocking(in_spi_hw, 0, bytes, 1) != 1)
    {
        gpio_put(in_cs, 1);
        return status::GET_CONTROL_BITS_ERROR;
    }

    gpio_put(in_cs, 1);

    return status::SUCCESS;
}

pico_canlib::status pico_canlib::reset()
{
    gpio_put(in_cs, 0);
    uint8_t data = (uint8_t)XL2515::SPI_INSTR_XL::RESET;
    if (spi_write_blocking(in_spi_hw, &data, 1) != 1)
    {
        gpio_put(in_cs, 1);
        return pico_canlib::status::RESET_ERROR;
    }
    gpio_put(in_cs, 1);
    return pico_canlib::status::SUCCESS;
}

pico_canlib::status pico_canlib::requestTS(uint8_t buffer)
{
    uint8_t data = (uint8_t)XL2515::SPI_INSTR_XL::REQTS | (1 << buffer);
    gpio_put(in_cs, 0);
    if (spi_write_blocking(in_spi_hw, &data, 1) != 1)
    {
        gpio_put(in_cs, 1);
        return pico_canlib::status::REQUESTTS_ERROR;
    }
    gpio_put(in_cs, 1);
    return pico_canlib::status::SUCCESS;
}

pico_canlib::status pico_canlib::checkStatus(uint8_t *status)
{
    uint8_t instr = (uint8_t)XL2515::SPI_INSTR_XL::READ_STATUS;
    gpio_put(in_cs, 0);
    spi_write_blocking(in_spi_hw, &instr, 1);
    if (spi_read_blocking(in_spi_hw, 0, status, 1) != 1)
    {
        gpio_put(in_cs, 1);
        return pico_canlib::status::STATUS_ERROR;
    }
    gpio_put(in_cs, 1);
    return pico_canlib::status::SUCCESS;
}

// bit modify helper used to set/clear individual bits in a register
pico_canlib::status pico_canlib::modifiedBit(uint8_t bytes, uint8_t address, uint8_t masked)
{
    XL2515::bit_modify msg;
    msg.addr = address;
    msg.mask = masked;
    msg.payload = bytes;

    gpio_put(in_cs, 0);
    if (spi_write_blocking(in_spi_hw, (uint8_t *)&msg, 4) != 4)
    {
        gpio_put(in_cs, 1);
        return pico_canlib::status::MODIFIED_ERROR;
    }
    gpio_put(in_cs, 1);
    return pico_canlib::status::SUCCESS;
}

pico_canlib::status pico_canlib::transmitCAN(XL2515::TX_BUFFER_SEL TX_SEL, uint32_t can_id, bool isExtended, uint8_t *data_buffer, uint8_t data_length, XL2515::PRIORITY priority)
{
    uint8_t TX_ID = (uint8_t)TX_SEL;
    // Set Data Length
    setByte(data_length, (XL2515::IN_ADDR)((uint8_t)XL2515::IN_ADDR::TXBxDLC + (TX_ID << 4))); // Weird ah implementation but we made do

    // Set ID
    if (isExtended)
    {
        setByte((can_id >> 16 | 0x4), (XL2515::IN_ADDR)((uint8_t)XL2515::IN_ADDR::TXBxSIDL + (TX_ID << 4)));
        setByte((can_id >> 8), (XL2515::IN_ADDR)((uint8_t)XL2515::IN_ADDR::TXBxEID8 + (TX_ID << 4)));
        setByte(can_id, (XL2515::IN_ADDR)((uint8_t)XL2515::IN_ADDR::TXBxEID0 + (TX_ID << 4)));
        setByte(0, (XL2515::IN_ADDR)((uint8_t)XL2515::IN_ADDR::TXBxSIDL + (TX_ID << 4)));
    }
    else
    {
        setByte(can_id >> 3, (XL2515::IN_ADDR)((uint8_t)XL2515::IN_ADDR::TXBxSIDH + (TX_ID << 4)));
        setByte((can_id << 5), (XL2515::IN_ADDR)((uint8_t)XL2515::IN_ADDR::TXBxSIDL + (TX_ID << 4)));
        setByte(0, (XL2515::IN_ADDR)((uint8_t)XL2515::IN_ADDR::TXBxEID0 + (TX_ID << 4)));
        setByte(0, (XL2515::IN_ADDR)((uint8_t)XL2515::IN_ADDR::TXBxEID8 + (TX_ID << 4)));
    }

    // Set Priority
    XL2515::bit_modify masked_message;
    masked_message.addr = (uint8_t)XL2515::IN_ADDR::TXBxCTRL + (TX_ID << 4);
    masked_message.mask = 0x03;
    masked_message.payload = (uint8_t)priority;

    gpio_put(in_cs, 0);
    if (spi_write_blocking(in_spi_hw, (uint8_t *)&masked_message, 4) != 4)
    {
        gpio_put(in_cs, 1);
        return pico_canlib::status::TX_PAYLOAD_COMMAND_ERROR;
    }
    gpio_put(in_cs, 1);

    // Set Data
    // XL2515::load_tx_buffer message;
    uint8_t instr = 0x41 + (TX_ID * 2);

    gpio_put(in_cs, 0);
    spi_write_blocking(in_spi_hw, &instr, 1);
    spi_write_blocking(in_spi_hw, data_buffer, data_length);
    // if (spi_write_blocking(in_spi_hw, (uint8_t *)&message, data_length + 1) != (data_length + 1))
    // {
    //     gpio_put(in_cs, 1);
    //     return pico_canlib::status::TX_PAYLOAD_COMMAND_ERROR;
    // }

    gpio_put(in_cs, 1);

    // Request to send CAN message
    return requestTS(TX_ID);
}

// flow:
// 1. poll READ_STATUS (0xb0) (part of main)
// 2. check if bit 0 and 1 of received byte
// 3. if bit is high, send READ RX BUFFER command
// 4. clear CANINTF via bitmodify
pico_canlib::status pico_canlib::receiveCAN(/*uint8_t rxstat, uint8_t RX_ID,*/ uint32_t *id, uint8_t *buffer, uint32_t idSize = 4, uint8_t bufferSize = 8)
{
    uint8_t st;
    uint8_t RX_ID;
    status errorCode = checkStatus(&st);
    printf("status: %d\n", st);
    if (st != 0x01 && st != 0x02 && st != 0x03)
    {
        return status::NO_NEW_MESSAGE;
    }
    if (st & 0x01)
    {
        RX_ID = 0x00;
    }
    if (st & 0x02)
    {
        RX_ID = 0x01;
    }
    // check bits 0 or 1 based on RX_ID filter
    uint8_t pendingMask = (RX_ID == 0) ? 0x01 : 0x02;
    if ((st & pendingMask) == 0)
    {
        return status::STATUS_STALL;
    }

    // READ RX BUFFER command + read
    uint8_t instr = (uint8_t)XL2515::SPI_INSTR_XL::READ_RX_BUFF | (RX_ID << 2);

    gpio_put(in_cs, 0);
    if (spi_write_blocking(in_spi_hw, &instr, 1) != 1)
    {
        gpio_put(in_cs, 1);
        return status::RX_ID_ERROR;
    }

    // reads ID, data length, buffer
    uint16_t totalBytes = (uint16_t)idSize + (uint16_t)bufferSize + 1;
    if (spi_read_blocking(in_spi_hw, 0, buffer, totalBytes) != totalBytes)
    {
        gpio_put(in_cs, 1);
        return status::RX_PAYLOAD_ERROR;
    }
    gpio_put(in_cs, 1);

    // converts buffer id
    uint32_t id = (buffer[0] << 3) | (buffer[1] >> 5);
    buffer[0] = (id >> 24) & 0xff;
    buffer[1] = (id >> 16) & 0xff;
    buffer[2] = (id >> 8) & 0xff;
    buffer[3] = (id) & 0xff;

    // clear interrupt CANINTF
    status c = modifiedBit(0, (uint8_t)XL2515::IN_ADDR::CANINTF, pendingMask);
    if (c != status::SUCCESS)
    {
        return c;
    }

    return status::SUCCESS;
}