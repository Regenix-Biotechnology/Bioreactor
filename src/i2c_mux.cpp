#include "i2c_mux.h"

/**
 * @brief Constructor to initialize the i2c mux variable. To default state
 */
I2CMux::I2CMux()
    : isInit(false), i2cBus(nullptr), selectedPortMask(0x00) {}

/**
 * @brief Initialise the i2c mux with the i2cBus sent in parameter
 *
 * @param i2cBus The I2C Wire to use for this sensor
 * @return eI2cMuxStatus I2C_MUX_STATUS_OK if successful else error code
 */
eI2cMuxStatus I2CMux::begin(TwoWire *i2cBus)
{
    if (i2cBus == nullptr)
        return I2C_MUX_STATUS_NULL_POINTER;

    this->i2cBus = i2cBus;
    this->isInit = true;

    return I2C_MUX_STATUS_OK;
}

/**
 * @brief Set the i2c bus from the mux that will be connected to the esp32
 *
 * @param bus The bus to listen and write to
 * @return eI2cMuxStatus I2C_MUX_STATUS_OK if set bus was successful else return error code
 */
eI2cMuxStatus I2CMux::setBus(uint8_t bus)
{
    if (!this->isInit)
        return I2C_MUX_STATUS_NOT_INITIALISED;
    if (bus >= MAX_NB_BUS)
        return I2C_MUX_STATUS_INVALID_I2C_BUS;

    // build the cmd
    uint8_t cmd = 0;
    cmd |= (1 << bus);

    // send cmd to mux
    this->i2cBus->beginTransmission(MUX_ADDR);
    this->i2cBus->write(cmd);
    uint8_t ret = this->i2cBus->endTransmission();
    if (ret != I2C_COMMUNICATION_SUCCESS)
        return I2C_MUX_STATUS_FAILED_TO_SEND_REQUEST;

    return I2C_MUX_STATUS_OK;
}

/**
 * @brief Read the current bus from the i2c mux and return the active bus
 *
 * @param pBus Pointer to return the bus from
 * @return eI2cMuxStatus I2C_MUX_STATUS_OK if reading current bus was successful else return error code
 */
eI2cMuxStatus I2CMux::getCurrentBus(uint8_t *pBus)
{
    if (!this->isInit)
        return I2C_MUX_STATUS_NOT_INITIALISED;
    if (pBus == nullptr)
        return I2C_MUX_STATUS_NULL_POINTER;

    *pBus = INVALID_I2C_BUS;

    readPortMaskForMux();

    if (this->selectedPortMask == 0)
        return I2C_MUX_STATUS_NO_BUS_SELECTED;

    uint8_t position = 0;
    uint8_t mask = this->selectedPortMask;
    while ((mask & 1) == 0)
    {
        // Shift until we find the 1
        mask >>= 1;
        position++;
    }
    *pBus = position;
    return I2C_MUX_STATUS_OK;
}

/**
 * @brief Ask the i2c mux what are the currently selected mux port and update this->selectedPortMask
 *
 */
void I2CMux::readPortMaskForMux()
{
    this->i2cBus->requestFrom(MUX_ADDR, REQ_SELECTED_PORT_MASK);
    uint8_t selectedPortMask = Wire.read();
    this->selectedPortMask = selectedPortMask;
}

/**
 * @brief Check if the i2c mux device is connected to the current i2c bus
 *
 * @return True if i2c mux device is detected else return false
 */
bool I2CMux::isConnected()
{
    if (!this->isInit)
        return false;

    this->i2cBus->beginTransmission(MUX_ADDR);
    uint8_t ret = this->i2cBus->endTransmission();
    return !ret;
}