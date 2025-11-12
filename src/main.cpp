#include "main.h"

void setup()
{
    Serial.begin(SERIAL_BAUDRATE);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("Hello, World!");

    beginBioreactorController();
}

void loop()
{
    switch (bioreactorState)
    {
    case eBioreactorState::IDLE:
        setFansState(OFF, OFF, OFF);
        // setPumpsSpeed(OFF, OFF, OFF, OFF);
        setValvesState(CLOSE, CLOSE, CLOSE, CLOSE, CLOSE);
        setPressureChamberState(OFF);
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
        // setPumpsSpeed(255, OFF, OFF, 255);
        setValvesState(CLOSE, CLOSE, CLOSE, CLOSE, CLOSE);
        setPressureChamberState(ON);
        setHeatersState(temperatureController.getHeaterPower(), temperatureController.isPatchHeatingNeeded());
        break;
    }
    default:
        /* code */
        break;
    }

    updateSensors();
    printBioreactorStateToSerial();
    updateTemperatureController();
    updatePressureChamberController();
    // updateBioreactorState(); // To be implemented when communication with the GUI will be available
    serialReader(); // This is used for DEBUG only
    cultureChamberPump2.update();
}
