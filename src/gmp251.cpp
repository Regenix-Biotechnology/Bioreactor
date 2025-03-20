#include "gmp251.h"

/**
 * @brief Constructor for GMP251 sensor.
 */
GMP251::GMP251(uint8_t rxPin, uint8_t txPin, uint8_t dePin, HardwareSerial &serial)
    : _rxPin(rxPin), _txPin(txPin), _dePin(dePin), _serial(serial), co2(0), lastReadTime(0) {}

/**
 * @brief Initializes RS-485 communication and forces serial mode.
 */
eGMP251Status GMP251::begin()
{
    _serial.begin(BAUD_RATE, SERIAL_8N1, _rxPin, _txPin);
    pinMode(_dePin, OUTPUT);
    digitalWrite(_dePin, LOW);

    return this->status = GMP_251_STATUS_NOT_INITIALISED;
}

eGMP251Status GMP251::update()
{
    if (millis() - this->lastReadTime >= READ_INTERVAL_MS)
    {
        this->lastReadTime = millis();

        if (this->status == GMP_251_STATUS_NOT_INITIALISED || this->status == GMP_251_STATUS_PARSING_FAILED)
        {
            forceSerialMode();
            return this->status;
        }

        parseCO2();
    }
    return this->status;
}

void GMP251::forceSerialMode()
{
    // Force Vaisala Industrial Protocol mode
    sendCarriageReturns();

    // Verify connection
    // The send is done after the read to remove the need for a delay between the two.
    String response = readResponse();
    sendCommand("send");

    if (response.isEmpty())
        this->status = GMP_251_STATUS_NOT_INITIALISED;
    else
        this->status = GMP_251_STATUS_INITIALIZED;
}

/**
 * @brief Sends five carriage returns to force serial mode.
 */
void GMP251::sendCarriageReturns()
{
    digitalWrite(_dePin, HIGH); // Enable TX mode
    for (uint8_t i = 0; i < NUM_CARRIAGE_RETURNS; i++)
    {
        _serial.print("\r");
    }
    _serial.flush();
    digitalWrite(_dePin, LOW); // Switch to RX mode
}

/**
 * @brief Sends a command to the sensor.
 */
void GMP251::sendCommand(const String &command)
{
    clearBuffer();

    digitalWrite(_dePin, HIGH);
    _serial.print(command + "\r");
    _serial.flush();
    digitalWrite(_dePin, LOW);
}

/**
 * @brief Reads response from the sensor with a timeout.
 */
String GMP251::readResponse()
{
    String response = "";
    while (_serial.available())
    {
        char c = _serial.read();
        response += c;
    }
    return response;
}

/**
 * @brief Parses CO₂ concentration from the sensor response.
 */
eGMP251Status GMP251::parseCO2()
{
    String response = readResponse();
    sendCommand("send"); // Request CO₂ data

    int start = response.indexOf("CO2=");
    int end = response.indexOf("ppm", start);

    if (start == -1 || end == -1)
        return this->status = GMP_251_STATUS_PARSING_FAILED;

    String co2Value = response.substring(start + CO2_STRING_LENGTH, end);

    if (!isDigit(co2Value[co2Value.length() - 2]))
        return this->status = GMP_251_STATUS_PARSING_NOT_A_NUMBER;

    this->co2 = co2Value.toFloat();
    return this->status = GMP_251_STATUS_OK;
}

float GMP251::getCO2()
{
    if (this->status == GMP_251_STATUS_OK)
        return this->co2;

    return 0;
}

// === Calibration Functions ===

void GMP251::calibrateCO2(uint32_t referencePpm)
{
    sendCommand("cco2 -hi " + String(referencePpm));
    delay(100);
    sendCommand("cco2 -save");
    Serial.println("CO₂ Calibration complete.");
}

void GMP251::setTemperatureCompensation(const String &mode)
{
    sendCommand("tcmode " + mode);
    Serial.println("Temperature compensation set to: " + mode);
}

void GMP251::calibrateTemperature(float temperature)
{
    sendCommand("env xtemp " + String(temperature));
    Serial.println("Temperature calibration complete.");
}

void GMP251::calibratePressure(float pressure)
{
    sendCommand("env xpres " + String(pressure));
    Serial.println("Pressure calibration complete.");
}

void GMP251::calibrateOxygen(float oxygen)
{
    sendCommand("env xoxy " + String(oxygen));
    Serial.println("Oxygen calibration complete.");
}

// === Utils function ===
void GMP251::clearBuffer()
{
    while (Serial.available())
    {
        Serial.read();
    }
}