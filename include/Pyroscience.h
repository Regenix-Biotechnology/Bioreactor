#ifndef PYROSCIENCE_H
#define PYROSCIENCE_H

#include <Arduino.h>

typedef enum
{
    PYROSCIENCE_STATUS_OK = 0,
    PYROSCIENCE_STATUS_NOT_INITIALISED,
    PYROSCIENCE_STATUS_INVALID_SERIAL_BUS,
    PYROSCIENCE_STATUS_FAILED_TO_SEND_REQUEST,
    PYROSCIENCE_STATUS_RX_MSG_TO_LONG,
    PYROSCIENCE_STATUS_READ_TIMEOUT,
    PYROSCIENCE_STATUS_NULL_POINTER,
    PYROSCIENCE_STATUS_DATA_NOT_FOUND,

    PYROSCIENCE_STATUS_MAX
} ePyroscienceStatus;

typedef struct __attribute__((packed))
{
    char cmd[4];
    float temperature;
    float oxygenDissous;
    float percentO2;
    float ph;
} sPyroscienceRxData;

/**
 * @class Pyroscience
 * @brief A class to interface with the Pyroscience FireSting sensor.
 *
 * This class allows reading temperature, pH, and dissolved oxygen data from the Pyroscience FireSting sensor over UART
 *
 * @note Pyroscience FireSting datasheet: https://www.pyroscience.com/en/downloads/laboratory-devices
 */
class Pyroscience
{
public:
    Pyroscience();
    ePyroscienceStatus begin(HardwareSerial *pSerial);
    float getLastTemperature() const;
    float getLastPH() const;
    float getLastDO() const;
    float getLastDOPercent() const;
    void getLastRxData(sPyroscienceRxData *data) const;
    ePyroscienceStatus fetchData();
    ePyroscienceStatus getData(sPyroscienceRxData *data);

private:
    static constexpr uint8_t REQUEST_MEASURE_SIZE = 9;
    static constexpr uint8_t MAX_RX_BUFF_SIZE = 128;
    static constexpr uint32_t TIMEOUT_READ = 1500;
    static constexpr uint32_t PYROSCIENCE_BAUDRATE = 115200;
    // static constexpr const char *REQUEST_MEASUREMENT_CMD;
    // static constexpr const char *FIRESTING_CHAN_NUM;
    // static constexpr const char *FIRESTING_MEASUREMENT_PARAM;
    // static constexpr const char *REQUEST_MEASUREMENT_CMD = "MEA";
    // static constexpr char REQUEST_MEASUREMENT_CMD[] = "MEA";
    // static constexpr char FIRESTING_CHAN_NUM[2];
    // static constexpr char FIRESTING_MEASUREMENT_PARAM[3];

    static constexpr uint8_t TEMP_POS_IN_MSG = 8;
    static constexpr uint8_t DO_POS_IN_MSG = 9;
    static constexpr uint8_t DO_PERCENT_POS_IN_MSG = 17;
    static constexpr uint8_t PH_POS_IN_MSG = 14;
    static constexpr float TEMP_DIVIDER = 1000.0;
    static constexpr float DO_DIVIDER = 100.0;
    static constexpr float DO_PERCENT_DIVIDER = 1000.0;
    static constexpr float PH_DIVIDER = 1000.0;

    ePyroscienceStatus parseRxData(sPyroscienceRxData *data, char *dataStr, uint8_t size);
    ePyroscienceStatus getFloatAt(char *dataStr, uint8_t pos, uint8_t size, float *resFloat);
    // float getFloatAt(char *dataStr, uint8_t pos, uint8_t size);
    ePyroscienceStatus read(char *buff, uint8_t *readSize);

    char txBuff[REQUEST_MEASURE_SIZE];
    char rxBuff[MAX_RX_BUFF_SIZE];
    uint32_t startReadTime;
    HardwareSerial *pSerial;
    sPyroscienceRxData data;
    bool isInit;
};

#endif