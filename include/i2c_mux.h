#ifndef I2CMUX
#define I2CMUX

#include <Wire.h>

typedef enum
{
    I2C_MUX_STATUS_OK = 0,
    I2C_MUX_STATUS_NO_BUS_SELECTED,
    I2C_MUX_STATUS_NULL_POINTER,
    I2C_MUX_STATUS_NOT_INITIALISED,
    I2C_MUX_STATUS_INVALID_I2C_BUS,
    I2C_MUX_STATUS_FAILED_TO_SEND_REQUEST,

    I2C_MUX_STATUS_MAX
} eI2cMuxStatus;

/**
 * @class I2CMux
 * @brief A class to interface with the i2c mux device.
 *
 * This class allows to switch between multiple i2c bus 8 total bus
 */
class I2CMux
{
public:
    I2CMux();
    eI2cMuxStatus begin(TwoWire *i2cBus);
    eI2cMuxStatus getCurrentBus(uint8_t *pBus);
    eI2cMuxStatus setBus(uint8_t bus);
    bool isConnected();

private:
    static constexpr uint8_t MUX_ADDR = 0x70;
    static constexpr uint8_t NB_BUS = 8;
    static constexpr uint8_t REQ_SETTINGS = 0x01;
    static constexpr uint8_t INVALID_I2C_BUS = 0xFF;
    static constexpr uint8_t I2C_COMMUNICATION_SUCCESS = 0;

    bool isInit;
    uint8_t settings;
    TwoWire *i2cBus;

    void updateSettings();
};

#endif // I2CMUX