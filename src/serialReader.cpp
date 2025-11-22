#include "serialReader.h"

void receiveSerialCommand()
{
    if (Serial.available())
    {
        String rx = Serial.readStringUntil('\n');

        uint8_t rx_buff[sizeof(float) * 6];
        if (rx == "STATE=APPROV")
        {
            setBioreactorState((uint8_t)eBioreactorState::APPROV);
        }
        if (rx == "STATE=HEAT")
        {
            setBioreactorState((uint8_t)eBioreactorState::PREPARE);
        }
        if (rx == "STATE=CULTURE")
        {
            setBioreactorState((uint8_t)eBioreactorState::RUN);
        }
        if (rx == "STATE=RETURN")
        {
            setBioreactorState((uint8_t)eBioreactorState::GROWTH_LIQUID_RETURN);
        }
        if (rx == "STATE=CLEANING")
        {
            setBioreactorState((uint8_t)eBioreactorState::CLEANING_LIQUID_APPROV);
        }
        if (rx == "STATE=CLEANING-CIRCUL")
        {
            setBioreactorState((uint8_t)eBioreactorState::CLEANING_CIRCULATION);
        }
        if (rx == "STATE=CLEANING-RETURN")
        {
            setBioreactorState((uint8_t)eBioreactorState::CLEANING_RETURN);
        }
        if (rx == "STATE=RINSING")
        {
            setBioreactorState((uint8_t)eBioreactorState::RINSING_LIQUID_APPROV);
        }
        if (rx == "STATE=RINSING-CIRCUL")
        {
            setBioreactorState((uint8_t)eBioreactorState::RINSING_CIRCULATION);
        }
        if (rx == "STATE=RINSING-RETURN")
        {
            setBioreactorState((uint8_t)eBioreactorState::RINSING_RETURN);
        }
        if (rx == "STATE=REDUCE-OVERFLOW")
        {
            setBioreactorState((uint8_t)eBioreactorState::REDUCE_OVERFLOW);
        }
        if (rx == "STATE=TEST")
        {
            setBioreactorState((uint8_t)eBioreactorState::TEST);
        }
        if (rx == "STATE=OPEN-VALVES")
        {
            setBioreactorState((uint8_t)eBioreactorState::OPEN_VALVES);
        }
        if (rx == "STATE=IDLE")
        {
            setBioreactorState((uint8_t)eBioreactorState::IDLE);
        }
        if (rx == "STATE=SAMPLING")
        {
            setBioreactorState((uint8_t)eBioreactorState::SAMPLING);
        }
        if (sscanf(rx.c_str(), "TEMP=%f", (float *)rx_buff))
        {
            float temp = *((float *)rx_buff);
            temperatureController.setReferenceTemperature(temp);
            bioreactorParameter.putFloat("temperature", temp);
        }
        if (sscanf(rx.c_str(), "PH=%f", (float *)rx_buff))
        {
            float ph = *((float *)rx_buff);
            bioreactorParameter.putFloat("ph", ph);
        }
        if (sscanf(rx.c_str(), "DO=%f", (float *)rx_buff))
        {
            float oxy_percent = *((float *)rx_buff);
            bioreactorParameter.putFloat("do", oxy_percent);
        }
        if (sscanf(rx.c_str(), "CO2=%f", (float *)rx_buff))
        {
            float CO2_PPM = *((float *)rx_buff);
            bioreactorParameter.putFloat("CO2", CO2_PPM);
            pressureChamber.setReferenceLevel(CO2, CO2_PPM);
            Serial.print("CO2 Reference Level updated to: ");
            Serial.println(CO2_PPM);
        }
        if (sscanf(rx.c_str(), "O2=%f", (float *)rx_buff))
        {
            float O2_PPM = *((float *)rx_buff);
            bioreactorParameter.putFloat("O2", O2_PPM);
            pressureChamber.setReferenceLevel(O2, O2_PPM);
            Serial.print("O2 Reference Level updated to: ");
            Serial.println(O2_PPM);
        }
        if (sscanf(rx.c_str(), "PUMP-SPEED=%f,%f,%f,%f", &(((float *)rx_buff)[0]), &(((float *)rx_buff)[1]), &(((float *)rx_buff)[2]), &(((float *)rx_buff)[3])))
        {
            // uint8_t i = 0;
            // float approvPumpSpeed = *((float *)(&(rx_buff[sizeof(float) * i++])));
            // float circulationPumpSpeed = *((float *)(&(rx_buff[sizeof(float) * i++])));
            // float cultureChamberPump1Speed = *((float *)(&(rx_buff[sizeof(float) * i++])));
            // float cultureChamberPump2Speed = *((float *)(&(rx_buff[sizeof(float) * i++])));
            // // set pump
            // // save pump
            // setPumpsSpeed(approvPumpSpeed, circulationPumpSpeed, cultureChamberPump1Speed, cultureChamberPump2Speed);
        }
        if (rx == "CALIB-PH=4")
        {
            pHSensor.calibrateSinglePoint(eCalibrationValues::CAL_PH_4);
        }
        if (rx == "CALIB-PH=7")
        {
            pHSensor.calibrateSinglePoint(eCalibrationValues::CAL_PH_7);
        }
        if (rx == "CALIB-PH=10")
        {
            pHSensor.calibrateSinglePoint(eCalibrationValues::CAL_PH_10);
        }
        if (rx == "CALIB-DO=0")
        {
            // do calib
        }
        if (rx == "CALIB-DO=100")
        {
            // do calib
        }
    }
}