#include "SHT40.h"

/**
 * @brief Constructor to initialize the SHT40 sensor variable to default state
 */
SHT40::SHT40(TwoWire *i2cBus)
    : isInit(false), temperature(0.0), humidity(0.0), i2cBus(i2cBus)
{
    memset(this->rxBuffer, 0, SHT40_RSP_SIZE);
}

/**
 * @brief Initialise the sht40 sensor
 *
 * @return eSHT40Status SHT40_STATUS_OK if init is succesful else return error code
 */
eSHT40Status SHT40::begin()
{
    if (i2cBus == nullptr)
        return SHT40_STATUS_INVALID_I2C_BUS;
    this->isInit = true;
    return SHT40_STATUS_OK;
}

/**
 * @brief Check if the SHT40 device is connected to the current i2c bus
 *
 * @return True if sht40 device is detected else return false
 */
bool SHT40::isConnected()
{
    if (this->isInit == false)
        return SHT40_STATUS_NOT_INITIALISED;

    this->i2cBus->beginTransmission(SHT40_ADDR);
    uint8_t ret = this->i2cBus->endTransmission();
    return !ret;
}

/**
 * @brief Get the last fetched temperature value from the sensor.
 * @return float The measured temperature in degrees Celsius.
 */
float SHT40::getLastTemperature() const
{
    return this->temperature;
}

/**
 * @brief Get the last fetched humidity value from the sensor.
 * @return float The measured humidity in % rH.
 */
float SHT40::getLastHumidity() const
{
    return this->humidity;
}

/**
 * @brief Fetch and update the sensor data for temperature and humidity. WARNING this task has a 10ms delay
 *
 * @return eSHT40Status SHT40_STATUS_OK if data is updated succesfully else return error code
 */
eSHT40Status SHT40::fetchData()
{
    if (!this->isInit)
        return SHT40_STATUS_NOT_INITIALISED;

    this->i2cBus->beginTransmission(SHT40_ADDR);
    this->i2cBus->write(SHT40_REQ_TEMP);
    uint8_t ret = this->i2cBus->endTransmission();
    if (ret != I2C_COMMUNICATION_SUCCESS)
        return SHT40_STATUS_FAILED_TO_SEND_REQUEST;

    delay(I2C_READ_DELAY);

    size_t recv = this->i2cBus->requestFrom(SHT40_ADDR, SHT40_RSP_SIZE);
    if (recv != SHT40_RSP_SIZE)
        return SHT40_STATUS_WRONG_MSG_LENGTH;

    // read msg
    memset(rxBuffer, 0, SHT40_RSP_SIZE);
    for (uint8_t i = 0; i < SHT40_RSP_SIZE; i++)
    {
        rxBuffer[i] = this->i2cBus->read();
    }

    // Check CRC
    if (rxBuffer[INDEX_CRC_TEMPERATURE] != crc8((&(rxBuffer[INDEX_TEMPERATURE])), RAW_TEMPERATURE_SIZE) ||
        rxBuffer[INDEX_CRC_HUMIDITY] != crc8((&(rxBuffer[INDEX_HUMIDITY])), RAW_HUMIDITY_SIZE))
        return SHT40_STATUS_INVALID_CRC;

    // Convert data
    float rawTemp = (((uint16_t)(rxBuffer[INDEX_TEMPERATURE])) << NB_BITS_IN_BYTE) + ((uint16_t)rxBuffer[INDEX_TEMPERATURE + 1]);
    float rawHumidity = (((uint16_t)(rxBuffer[INDEX_HUMIDITY])) << NB_BITS_IN_BYTE) + ((uint16_t)rxBuffer[INDEX_HUMIDITY + 1]);

    this->temperature = -45 + 175 * rawTemp / 65535; // Calculation from datasheet
    this->humidity = -6 + 125 * rawHumidity / 65535; // Calculation from datasheet

    return SHT40_STATUS_OK;
}

/**
 * @brief Fetch and return the sensor data for temperature and humidity. WARNING this task has a 10ms delay
 *
 * @param temperature Output temperature measured can be nullptr if only humidity needed
 * @param humidity Output humidity measured can be nullptr if only temperature needed
 * @return eSHT40Status SHT40_STATUS_OK if data is updated succesfully else return error code
 *
 * @warning This function has a 10ms delay
 */
eSHT40Status SHT40::getData(float *temperature, float *humidity)
{
    eSHT40Status status = this->fetchData();
    if (status != SHT40_STATUS_OK)
        return status;

    if (temperature)
        *temperature = this->temperature;

    if (humidity)
        *humidity = this->humidity;

    return SHT40_STATUS_OK;
}

/**
 * @brief getData(float *temperature, float *humidity) overloading fetch and return the sensor data for temperature and humidity. WARNING this task has a 10ms delay
 *
 * @param temperature Output temperature measured
 * @return eSHT40Status SHT40_STATUS_OK if data is updated succesfully else return error code
 *
 * @warning This function has a 10ms delay
 */
eSHT40Status SHT40::getData(float *temperature)
{
    return this->getData(temperature, nullptr);
}

/**
 * @brief will return a unique uint8_t number for the provided data
 * CRC-8 formula from page 14 of SHT spec pdf
 *
 * Test data 0xBE, 0xEF should yield 0x92
 *
 * Initialization data 0xFF
 * Polynomial 0x31 (x8 + x5 +x4 +1)
 * Final XOR 0x00
 *
 * @param data  data to calculate crc from
 * @param len   length of data to calculate crc from
 * @return uint8_t  result of crc8
 */
uint8_t SHT40::crc8(const uint8_t *data, int len)
{
    const uint8_t POLYNOMIAL(0x31);
    uint8_t crc(0xFF);

    for (uint8_t j = len; j; --j)
    {
        crc ^= *data++;

        for (uint8_t i = 8; i; --i)
        {
            crc = (crc & 0x80) ? (crc << 1) ^ POLYNOMIAL : (crc << 1);
        }
    }
    return crc;
}