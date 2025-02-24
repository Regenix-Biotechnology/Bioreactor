#include "main.h"

void setup()
{
    Serial.begin(SERIAL_BAUDRATE);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("Hello, World!");

    beginBioreactorController();
    // Serial.printf("enable ret: %d\n", ret);
}

void loop()
{
    switch (bioreactorState)
    {
    case eBioreactorState::IDLE:
        setFansState(OFF, OFF, OFF);
        setPumpsSpeed(OFF, OFF, OFF, OFF);
        setValvesState(CLOSE, CLOSE, CLOSE, CLOSE, CLOSE);
        setPressureChamberValvesState(CLOSE, CLOSE, CLOSE, CLOSE);
        setHeatersState(OFF, OFF);
        break;
    case eBioreactorState::APPROV:
        /* code */
        break;
    case eBioreactorState::PREPARE:
        /* code */
        break;
    case eBioreactorState::RUN:
        /* code */
        break;
    case eBioreactorState::TEST: // For the fluidic and heating system test
    {
        setFansState(ON, OFF, OFF);
        setPumpsSpeed(PUMP_MAX_SPEED / 8, OFF, OFF, OFF);
        setValvesState(CLOSE, CLOSE, CLOSE, CLOSE, CLOSE);
        setPressureChamberValvesState(CLOSE, CLOSE, CLOSE, CLOSE);
        setHeatersState(temperatureController.getHeaterPower(), temperatureController.isPatchHeatingNeeded());

        /******** for testing mux ********/
        muxI2c.setBus(1);
        float temperature1 = 0.0f;
        sht40.getData(&temperature1);

        muxI2c.setBus(2);
        float temperature2 = 0.0f;
        sht40.getData(&temperature2);

        Serial.printf(">temp bus 1: %f\n>temp bus 2: %f\n", temperature1, temperature2);
        break;
    }
    default:
        /* code */
        break;
    }

    // delay(1000);

    updateTemperatureController();

    muxI2c.setBus(0);
    heater.update();
    // updatePressureChamberController();
    // updateBioreactorState();
}
