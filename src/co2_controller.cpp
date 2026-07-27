#include "co2_controller.h"
#include <Wire.h>
#include "ledI2C.h"

CO2Controller::CO2Controller()
    : co2Ref(800.0f),
      co2MinRef(800.0f - 50.0f),
      co2MaxRef(800.0f + 50.0f)
{
}
/**
 * @brief Calculates the time the valve should remain open based on the concentrations of gases and the pressure.
 * @param co2Concentration The concentration of CO2 in the chamber in ppm.
 * @param pressure The pressure in the chamber in Pa.
 */
void CO2Controller::update(float co2Concentration)
{

    float o2ValveTime = 0;
    float co2ValveTime = 0;
    float airValveTime = 0;

    float co2Error = co2Concentration - this->co2Ref;

    // --- CO₂ Control Logic ---
    if (co2Error > 0)
    {
        float effectiveCO2Error = co2Error / co2Concentration;
        float airValveTimeFromCo2 = calculateTimeBeforeClosingValve(effectiveCO2Error) * CORRECTION_FACTOR_CO2_REDUCTION;
        airValveTime = max(airValveTime, airValveTimeFromCo2);
    }
    else if (co2Error < 0)
    {
        float correctionFactor = (-co2Error < SMALL_CO2_ERROR_THRESHOLD)
                                     ? CORRECTION_FACTOR_CO2_SMALL
                                     : CORRECTION_FACTOR_CO2;

        // Serial.println(">CO2 Error: " + String(co2Error));
        // Serial.println(">CO2 Error Correction Factor: " + String(correctionFactor));

        // co2ValveTime = calculateTimeBeforeClosingValve(-co2Error) * correctionFactor;
    }

    // Apply the calculated times
    // this->timeBeforeClosingCO2Valve = millis() + static_cast<unsigned long>(co2ValveTime);

    // --- DEBUG OUTPUT ---
    // float printTimeBeforeClosingO2Valve = (timeBeforeClosingO2Valve - millis());
    // if (printTimeBeforeClosingO2Valve > 999999999)
    //     printTimeBeforeClosingO2Valve = 0;
    // float printTimeBeforeClosingCO2Valve = (timeBeforeClosingCO2Valve - millis());
    // if (printTimeBeforeClosingCO2Valve > 999999999)
    //     printTimeBeforeClosingCO2Valve = 0;
    // float printTimeBeforeClosingAirValve = (timeBeforeClosingAirValve - millis());
    // if (printTimeBeforeClosingAirValve > 999999999)
    //     printTimeBeforeClosingAirValve = 0;
    // Serial.println(">O2 Opening Time: " + String(printTimeBeforeClosingO2Valve));
    // Serial.println(">CO2 Opening Time: " + String(printTimeBeforeClosingCO2Valve));
    // Serial.println(">Air Opening Time: " + String(printTimeBeforeClosingAirValve));
}
/**
 * @brief Calculates the time before closing the valve based on the error.
 * @param error The error value.
 * @return The time before closing the valve in milliseconds.
 * @note This function makes a few simplifications that must be tested :
 * - The flow rate is assumed to be constant even if opening the valve will increase the pressure in the chamber.
 * - The equation assumes that no gas is lost when new gas is added to the chamber, wich is false.
 *   Because we will have multiple adjustment in a row, I assume that the error will disapear after a few iterations.
 */
float CO2Controller::calculateTimeBeforeClosingValve(float error)
{
    float timeBeforeClosing = 0;
    float q = 0.0f;           // Flow rate in liters per second
    float volumeToAdd = 0.0f; // Volume to add in liters

    q = (PI * pow(R, 4)) / (8 * MU_CO2 * L) * (P_B_APPROV - P_B_CHAMBER); // Flow rate equation for CO2 (Poiseuille's law)
    volumeToAdd = error * PPM_TO_LITERS;                                  // Volume to add in liters
    timeBeforeClosing = volumeToAdd / q * SECONDS_TO_MILLIS;              // Time in milliseconds (t=V/Q)

    return timeBeforeClosing;
}

/**
 * @brief Sets the reference level for the specified valve.
 * @param ReferenceLevel The reference level to set.
 */
void CO2Controller::setReferenceLevel(float ReferenceLevel)
{
    this->co2Ref = ReferenceLevel;
    this->co2MinRef = ReferenceLevel - CO2_DEAD_ZONE;
    this->co2MaxRef = ReferenceLevel + CO2_DEAD_ZONE;
}

float CO2Controller::GetCO2Value()
{
    // break;
}