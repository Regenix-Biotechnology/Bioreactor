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
        setFansState(OFF, OFF, OFF, OFF, OFF, OFF, OFF);
        // setPumpsSpeed(OFF, OFF, OFF, OFF);
        setValvesState(CLOSE, CLOSE, CLOSE);
        setPressureChamberValvesState(OFF, OFF, OFF);
        setHeatersState(OFF);
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
        setFansState(ON, ON, ON, ON, ON, ON, ON);
        // setPumpsSpeed(255, OFF, OFF, 255);
        setValvesState(CLOSE, CLOSE, CLOSE);
        setPressureChamberValvesState(OFF, OFF, OFF);
        setPressureChamberState(ON);
        setHeatersState(temperatureController.getHeaterPower());
        break;
    }
    default:
        /* code */
        break;
    }

    updateTemperatureController();
    updatePressureChamberController();
    // updateBioreactorState(); // To be implemented when communication with the GUI will be available
    serialReader(); // This is used for DEBUG only
    cultureChamberPump2.update();
}
