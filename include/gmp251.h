#ifndef GMP251_H
#define GMP251_H

#include <Arduino.h>
#include <HardwareSerial.h>

typedef enum
{
    GMP_251_STATUS_OK = 0,
    GMP_251_STATUS_INITIALIZED,
    GMP_251_STATUS_NOT_INITIALISED,
    GMP_251_STATUS_FAILED_TO_SEND_REQUEST,
    GMP_251_STATUS_PARSING_FAILED,
    GMP_251_STATUS_PARSING_NOT_A_NUMBER,

    GMP_251_STATUS_MAX
} eGMP251Status;

/**
 * @brief GMP251 Carbon Dioxide Sensor Driver (RS-485 Communication).
 * @details Communicates with the Vaisala GMP251 CO₂ sensor using the Vaisala Industrial Protocol.
 * @link https://docs.vaisala.com/v/u/M211799EN-G/en-US
 */
class GMP251
{
public:
    GMP251(uint8_t rxPin, uint8_t txPin, uint8_t dePin, HardwareSerial &serial);
    eGMP251Status begin();
    eGMP251Status update();
    float getCO2();
    eGMP251Status getStatus() { return status; }
    void calibrateCO2(uint32_t referencePpm);
    void calibrateTemperature(float temperature);
    void calibratePressure(float pressure);
    void calibrateOxygen(float oxygen);
    void setTemperatureCompensation(const String &mode);

private:
    eGMP251Status parseCO2();
    String readResponse();
    void forceSerialMode();
    void sendCarriageReturns();
    void sendCommand(const String &command);
    void clearBuffer();

    HardwareSerial _serial;
    uint8_t _rxPin, _txPin, _dePin;
    uint32_t lastReadTime;
    eGMP251Status status;
    float co2;
    bool isGMP251Enabled;
    uint32_t sendCommandTime;
    bool isCommandSent;

    // Constants
    static constexpr uint8_t NUM_CARRIAGE_RETURNS = 5;
    static constexpr uint32_t RESPONSE_TIMEOUT_MS = 1000;
    static constexpr uint32_t READ_INTERVAL_MS = 500;
    static constexpr uint32_t BAUD_RATE = 19200;
    static constexpr uint8_t CO2_STRING_LENGTH = 4; // "CO2="
};

#endif // GMP251_H
