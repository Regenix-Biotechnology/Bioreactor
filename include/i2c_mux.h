#ifndef I2CMUX
#define I2CMUX

#include <Wire.h>

// typedef enum
// {
//     SHT40_STATUS_OK = 0,
//     SHT40_STATUS_NOT_INITIALISED,
//     SHT40_STATUS_INVALID_I2C_BUS,
//     SHT40_STATUS_FAILED_TO_SEND_REQUEST,
//     SHT40_STATUS_INVALID_CRC,
//     SHT40_STATUS_WRONG_MSG_LENGTH,

//     SHT40_STATUS_MAX
// } eSHT40Status;

/**
 * @class SHT40
 * @brief A class to interface with the SHT40 temperature and humidity sensor.
 *
 * This class allows reading temperature and humidity data from the SHT40 sensor over I2C.
 *
 * @note SHT40 datasheet: https://sensirion.com/media/documents/33FD6951/662A593A/HT_DS_Datasheet_SHT4x.pdf
 */
class I2CMux
{
public:
    I2CMux();
    void begin(TwoWire *i2cBus);
    // uint8_t getCurrentBus() const;
    // void setBus(uint8_t bus);
    bool enableMuxPort(uint8_t portNumber);
    bool disableMuxPort(uint8_t portNumber);
    bool isConnected();

private:
    // static constexpr uint8_t SHT40_RSP_SIZE = 6;
    static constexpr uint8_t MUX_ADDR = 0x70;
    static constexpr uint8_t NB_BUS = 8;
    // static constexpr uint8_t SHT40_REQ_TEMP = 0xFD;
    // static constexpr uint8_t RAW_TEMPERATURE_SIZE = 2;
    // static constexpr uint8_t RAW_HUMIDITY_SIZE = 2;
    // static constexpr uint8_t INDEX_CRC_TEMPERATURE = 2;
    // static constexpr uint8_t INDEX_CRC_HUMIDITY = 5;
    // static constexpr uint8_t INDEX_TEMPERATURE = 0;
    // static constexpr uint8_t INDEX_HUMIDITY = 3;
    // static constexpr uint8_t NB_BITS_IN_BYTE = 8;
    static constexpr uint8_t I2C_COMMUNICATION_SUCCESS = 0;
    static constexpr uint8_t I2C_READ_DELAY = 10;

    // uint8_t crc8(const uint8_t *data, int len);

    // uint8_t rxBuffer[SHT40_RSP_SIZE];
    bool isInit;
    // float temperature;
    // float humidity;
    TwoWire *i2cBus;
};

#endif // I2CMUX