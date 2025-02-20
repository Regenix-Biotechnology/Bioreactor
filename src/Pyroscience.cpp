#include "Pyroscience.h"

constexpr char REQUEST_MEASUREMENT_CMD[] = "MEA";
constexpr char FIRESTING_CHAN_NUM[] = "4";
constexpr char FIRESTING_MEASUREMENT_PARAM[] = "47";

/**
 * @brief Constructor to initialize the Pyroscience sensor
 */
Pyroscience::Pyroscience()
    : isInit(false),
      pSerial(nullptr),
      startReadTime(0)
{
    memset(this->rxBuff, 0, MAX_RX_BUFF_SIZE);
    memset(this->txBuff, 0, REQUEST_MEASURE_SIZE);
    memset(&(this->data), 0, sizeof(this->data));
}

/**
 * @brief Start pyroscience and pass serial uart interface (not yet initialised)
 *
 * @param pSerial Pointer to the uninitialised uart interface
 * @return ePyroscienceStatus If everything is ok PYROSCIENCE_STATUS_OK else return error code
 */
ePyroscienceStatus Pyroscience::begin(HardwareSerial *pSerial)
{
    if (pSerial == nullptr)
        return PYROSCIENCE_STATUS_INVALID_SERIAL_BUS;

    pSerial->begin(PYROSCIENCE_BAUDRATE);
    this->pSerial = pSerial;
    this->isInit = true;
    return PYROSCIENCE_STATUS_OK;
}

/**
 * @brief Get the last fetched temperature value from the sensor.
 * @return The measured temperature in degrees Celsius.
 */
float Pyroscience::getLastTemperature() const
{
    return this->data.temperature;
}

/**
 * @brief Get the last fetched pH value from the sensor.
 * @return The measured pH value.
 */
float Pyroscience::getLastPH() const
{
    return this->data.pH;
}

/**
 * @brief Get the last fetched dissolved oxygen value from the sensor.
 * @return The measured dissolved oxygen value in mg/L.
 */
float Pyroscience::getLastDO() const
{
    return this->data.dissolvedOxygen;
}

/**
 * @brief Get the last fetched dissolved oxygen value from the sensor in percentage.
 * @return The measured dissolved oxygen value in %.
 */
float Pyroscience::getLastDOPercent() const
{
    return this->data.percentO2;
}

/**
 * @brief Return the last fetched sPyroscienceRxData containing all the info
 *
 * @param pData The sPyroscienceRxData containing last fetched data
 */
void Pyroscience::getLastRxData(sPyroscienceRxData *pData) const
{
    memcpy(pData, &(this->data), sizeof(this->data));
}

/**
 * @brief Fetch the data and return a sPyroscienceRxData containing all the received value
 *
 * @param pData Pointer to a sPyroscienceRxData that will be filled with the reiceived data
 * @return ePyroscienceStatus If getData was a success return PYROSCIENCE_STATUS_OK else return error code
 */
ePyroscienceStatus Pyroscience::getData(sPyroscienceRxData *pData)
{
    if (!this->isInit)
        return PYROSCIENCE_STATUS_NOT_INITIALISED;

    if (pData == nullptr)
        return PYROSCIENCE_STATUS_NULL_POINTER;

    ePyroscienceStatus status = PYROSCIENCE_STATUS_OK;
    status = fetchData();
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    memcpy(pData, &(this->data), sizeof(this->data));

    return PYROSCIENCE_STATUS_OK;
}

/**
 * @brief Fetch and update the sensor data for temperature, pH, and dissolved oxygen.
 *
 * @return ePyroscienceStatus If fetchData was a success return PYROSCIENCE_STATUS_OK else return error code
 */
ePyroscienceStatus Pyroscience::fetchData()
{
    ePyroscienceStatus status;

    if (!this->isInit)
        return PYROSCIENCE_STATUS_NOT_INITIALISED;

    memset(rxBuff, 0, MAX_RX_BUFF_SIZE);

    // Create message
    sprintf(txBuff, "%s %s %s\r", REQUEST_MEASUREMENT_CMD, FIRESTING_CHAN_NUM, FIRESTING_MEASUREMENT_PARAM);

    // Send message
    size_t txSize = pSerial->print(txBuff);
    if (txSize != REQUEST_MEASURE_SIZE)
        return PYROSCIENCE_STATUS_FAILED_TO_SEND_REQUEST;

    // Read response
    uint8_t rxSize = 0;
    status = read(rxBuff, &rxSize);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    // Parse message
    status = parseRxData(&(this->data), rxBuff, rxSize);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    return PYROSCIENCE_STATUS_OK;
}

/**
 * @brief Read pyroscience response and return it in a buffer
 *
 * @param buff The message response from pyroscience
 * @param pReadSize The length of the response from pyroscience
 * @return ePyroscienceStatus PYROSCIENCE_STATUS_OK if read was a success else error message
 */
ePyroscienceStatus Pyroscience::read(char *buff, uint8_t *pReadSize)
{
    if (pReadSize == nullptr)
        return PYROSCIENCE_STATUS_NULL_POINTER;

    uint32_t startReadTime = millis();
    uint8_t index = 0;
    while (true)
    {
        // Verification
        if (index > MAX_RX_BUFF_SIZE - 1)
            return PYROSCIENCE_STATUS_RX_MSG_TO_LONG;

        if (startReadTime + TIMEOUT_READ < millis())
            return PYROSCIENCE_STATUS_READ_TIMEOUT;

        // Read data
        buff[index] = this->pSerial->read();

        // End of transmission
        if (buff[index] == '\r')
            break;

        index++;
    }
    *pReadSize = index;

    return PYROSCIENCE_STATUS_OK;
}

/**
 * @brief Parse the data from dataStr into a sPyroscienceRxData structure
 *
 * @param pData The structure to place the data in
 * @param dataStr The data string received from the pyroscience device
 * @param size The size of the received string
 * @return ePyroscienceStatus PYROSCIENCE_STATUS_OK if parsing was a success else error message
 */
ePyroscienceStatus Pyroscience::parseRxData(sPyroscienceRxData *pData, char *dataStr, uint8_t size)
{
    // Read command name
    memcpy(pData->cmd, dataStr, 3);
    pData->cmd[4] = '\n';
    ePyroscienceStatus status;

    // Read temperature
    float rawTemperature;
    status = getFloatAt(dataStr, TEMP_POS_IN_MSG, size, &rawTemperature);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    // Read pH
    float rawPh;
    status = getFloatAt(dataStr, PH_POS_IN_MSG, size, &rawPh);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    // Read DO
    float rawDo;
    status = getFloatAt(dataStr, DO_POS_IN_MSG, size, &rawDo);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    // Read Do percent
    float rawDoPercent;
    status = getFloatAt(dataStr, DO_PERCENT_POS_IN_MSG, size, &rawDoPercent);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    pData->temperature = rawTemperature / TEMP_DIVIDER;
    pData->dissolvedOxygen = rawDo / DO_DIVIDER;
    pData->percentO2 = rawDoPercent / DO_PERCENT_DIVIDER;
    pData->pH = rawPh / PH_DIVIDER;

    return PYROSCIENCE_STATUS_OK;
}

/**
 * @brief Convert the word a pos to a float and return it in pResFloat
 *
 * @param dataStr The data string received from the pyroscience device
 * @param pos The position of the word to convert to float
 * @param size The size of the received string
 * @param pResFloat The returned float data at the position specified
 * @return ePyroscienceStatus PYROSCIENCE_STATUS_OK if float was found else error message
 */
ePyroscienceStatus Pyroscience::getFloatAt(char *dataStr, uint8_t pos, uint8_t size, float *pResFloat)
{
    if (pResFloat == nullptr)
        return PYROSCIENCE_STATUS_NULL_POINTER;

    uint8_t wordCount = 0;
    uint8_t indexStartWord = 0;

    for (uint8_t i = 0; i < size; i++)
    {
        // Find a space character or end of string
        if (dataStr[i] == ' ' || dataStr[i] == '\r' || dataStr[i] == '\n')
        {
            wordCount++;

            // Found the space just before the word
            if (wordCount == pos)
            {
                indexStartWord = i + 1;
            }
            // Found the end of the word
            else if (wordCount == pos + 1)
            {
                // Convert string to float
                char *pEnd = &(dataStr[i]);
                *pResFloat = strtof(&(dataStr[indexStartWord]), &pEnd);
                return PYROSCIENCE_STATUS_OK;
            }
        }
    }

    // No word at position pos found
    return PYROSCIENCE_STATUS_DATA_NOT_FOUND;
}
