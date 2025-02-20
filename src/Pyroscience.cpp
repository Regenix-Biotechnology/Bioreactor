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
 * @brief start pyroscience and pass serial uart interface (not yet initialised)
 *
 * @param pSerial pointer to the uninitialised uart interface
 * @return ePyroscienceStatus if everything is ok PYROSCIENCE_STATUS_OK else return error code
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
    return this->data.ph;
}

/**
 * @brief Get the last fetched dissolved oxygen value from the sensor.
 * @return The measured dissolved oxygen value in mg/L.
 */
float Pyroscience::getLastDO() const
{
    return this->data.oxygenDissous;
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
 * @brief return the last fetched sPyroscienceRxData containing all the info
 *
 * @param pData the sPyroscienceRxData containing last fetched data
 */
void Pyroscience::getLastRxData(sPyroscienceRxData *pData) const
{
    memcpy(pData, &(this->data), sizeof(this->data));
}

/**
 * @brief fetch the data and return a sPyroscienceRxData containing all the received value
 *
 * @param pData pointer to a sPyroscienceRxData that will be filled with the reiceived data
 * @return ePyroscienceStatus if getData was a success return PYROSCIENCE_STATUS_OK else return error code
 */
ePyroscienceStatus Pyroscience::getData(sPyroscienceRxData *pData)
{
    if (this->isInit == false)
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
 * @return ePyroscienceStatus if fetchData was a success return PYROSCIENCE_STATUS_OK else return error code
 */
ePyroscienceStatus Pyroscience::fetchData()
{
    ePyroscienceStatus status = PYROSCIENCE_STATUS_OK;

    if (this->isInit == false)
        return PYROSCIENCE_STATUS_NOT_INITIALISED;

    memset(rxBuff, 0, MAX_RX_BUFF_SIZE);

    // create message
    sprintf(txBuff, "%s %s %s\r", REQUEST_MEASUREMENT_CMD, FIRESTING_CHAN_NUM, FIRESTING_MEASUREMENT_PARAM);

    // send message
    size_t txSize = pSerial->print(txBuff);
    if (txSize != REQUEST_MEASURE_SIZE)
        return PYROSCIENCE_STATUS_FAILED_TO_SEND_REQUEST;

    // read response
    uint8_t rxSize = 0;
    status = read(rxBuff, &rxSize);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    // parse message
    status = parseRxData(&(this->data), rxBuff, rxSize);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    return PYROSCIENCE_STATUS_OK;
}

/**
 * @brief read pyroscience response and return it in a buffer
 *
 * @param buff the message response from pyroscience
 * @param pReadSize the length of the response from pyroscience
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
        // verification
        if (index > MAX_RX_BUFF_SIZE - 1)
            return PYROSCIENCE_STATUS_RX_MSG_TO_LONG;

        if (startReadTime + TIMEOUT_READ < millis())
            return PYROSCIENCE_STATUS_READ_TIMEOUT;

        // read data
        buff[index] = this->pSerial->read();

        // end of transmission
        if (buff[index] == '\r')
            break;

        index++;
    }
    *pReadSize = index;

    return PYROSCIENCE_STATUS_OK;
}

/**
 * @brief parse the data from dataStr into a sPyroscienceRxData structure
 *
 * @param pData the structure to place the data in
 * @param dataStr the data string received from the pyroscience device
 * @param size the size of the received string
 * @return ePyroscienceStatus PYROSCIENCE_STATUS_OK if parsing was a success else error message
 */
ePyroscienceStatus Pyroscience::parseRxData(sPyroscienceRxData *pData, char *dataStr, uint8_t size)
{
    // read command name
    memcpy(pData->cmd, dataStr, 3);
    pData->cmd[4] = '\n';
    ePyroscienceStatus status;

    // read temperature
    float rawTemperature;
    status = getFloatAt(dataStr, TEMP_POS_IN_MSG, size, &rawTemperature);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    // read pH
    float rawPh;
    status = getFloatAt(dataStr, PH_POS_IN_MSG, size, &rawPh);
    if (status != PYROSCIENCE_STATUS_OK)
        return status;

    // read DO
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
    pData->oxygenDissous = rawDo / DO_DIVIDER;
    pData->percentO2 = rawDoPercent / DO_PERCENT_DIVIDER;
    pData->ph = rawPh / PH_DIVIDER;

    return PYROSCIENCE_STATUS_OK;
}

/**
 * @brief Convert the word a pos to a float and return it in pResFloat
 *
 * @param dataStr the data string received from the pyroscience device
 * @param pos the position of the word to convert to float
 * @param size the size of the received string
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
        if (dataStr[i] == ' ' || dataStr[i] == '\r' || dataStr[i] == '\n')
        {
            wordCount++;
            if (wordCount == pos)
            {
                indexStartWord = i + 1;
            }
            else if (wordCount == pos + 1)
            {
                char *pEnd = &(dataStr[i]);
                *pResFloat = strtof(&(dataStr[indexStartWord]), &pEnd);
                return PYROSCIENCE_STATUS_OK;
            }
        }
    }

    return PYROSCIENCE_STATUS_DATA_NOT_FOUND;
}
