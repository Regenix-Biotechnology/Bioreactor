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
 * @brief A class to interface with the i2c mux device BOB-16784
 * @note Datasheet: https://cdn.sparkfun.com/assets/5/4/1/3/9/Qwiic_Mux_Dimensions.pdf
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

    static constexpr uint8_t MAX_NB_BUS = 8;
    static constexpr uint8_t INVALID_I2C_BUS = 0xFF;

private:
    static constexpr uint8_t MUX_ADDR = 0x70;
    static constexpr uint8_t REQ_SELECTED_PORT_MASK = 0x01;
    static constexpr uint8_t I2C_COMMUNICATION_SUCCESS = 0;

    bool isInit;
    uint8_t selectedPortMask; // Mask for the i2c port selected ex: 0b00000010 means port 6
    TwoWire *i2cBus;

    void readPortMaskForMux();
};

#endif // I2CMUX