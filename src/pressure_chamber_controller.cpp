#include "pressure_chamber_controller.h"

/**
 * @brief Constructor to initialize the control loop parameters.
 */
PressureChamberController::PressureChamberController()
    : co2MinRef(CO2_REF - CO2_DEAD_ZONE),
      co2MaxRef(CO2_REF + CO2_DEAD_ZONE),
      co2Ref(CO2_REF),
      o2MinRef(O2_REF - O2_DEAD_ZONE),
      o2MaxRef(O2_REF + O2_DEAD_ZONE),
      o2Ref(O2_REF),
      o2ValveState(false),
      co2ValveState(false),
      airValveState(false),
      safetyValveState(false),
      pressureChamberState(false),
      timeBeforeClosingO2Valve(0),
      timeBeforeClosingCO2Valve(0),
      timeBeforeClosingAirValve(0)
{
}

/**
 * @brief Calculates the time the valve should remain open based on the concentrations of gases and the pressure.
 * @param o2Concentration The concentration of O2 in the chamber in %.
 * @param co2Concentration The concentration of CO2 in the chamber in ppm.
 * @param pressure The pressure in the chamber in Pa.
 */
void PressureChamberController::update(float o2Concentration, float co2Concentration, float pressure)
{
    if (!this->pressureChamberState)
        return;

    float o2ValveTime = 0;
    float co2ValveTime = 0;
    float airValveTime = 0;

    float o2Error = o2Concentration - this->o2Ref;
    float co2Error = co2Concentration - this->co2Ref;

    // --- O₂ Control Logic ---
    if (o2Error > 0)
    {
        float effectiveO2Error = o2Error / o2Concentration;
        airValveTime = calculateTimeBeforeClosingValve(AIR, effectiveO2Error) * CORRECTION_FACTOR_O2_REDUCTION;
    }
    else if (o2Error < 0)
    {
        float correctionFactor = (-o2Error < SMALL_O2_ERROR_THRESHOLD)
                                     ? CORRECTION_FACTOR_O2_SMALL
                                     : CORRECTION_FACTOR_O2;

        o2ValveTime = calculateTimeBeforeClosingValve(O2, -o2Error) * correctionFactor;
    }

    // --- CO₂ Control Logic ---
    if (co2Error > 0)
    {
        float effectiveCO2Error = co2Error / co2Concentration;
        float airValveTimeFromCo2 = calculateTimeBeforeClosingValve(AIR, effectiveCO2Error) * CORRECTION_FACTOR_CO2_REDUCTION;
        airValveTime = max(airValveTime, airValveTimeFromCo2);
    }
    else if (co2Error < 0)
    {
        float correctionFactor = (-co2Error < SMALL_CO2_ERROR_THRESHOLD)
                                     ? CORRECTION_FACTOR_CO2_SMALL
                                     : CORRECTION_FACTOR_CO2;

        // Serial.println(">CO2 Error: " + String(co2Error));
        // Serial.println(">CO2 Error Correction Factor: " + String(correctionFactor));

        co2ValveTime = calculateTimeBeforeClosingValve(CO2, -co2Error) * correctionFactor;
    }

    // --- Pressure Control Logic ---
    if (pressure < P_CHAMBER_MIN)
    {
        float airValveTimeFromPressure = millis() + AIR_VALVE_OPEN_TIME;
        airValveTime = max(airValveTime, airValveTimeFromPressure);
    }

    // Apply the calculated times
    this->timeBeforeClosingO2Valve = millis() + static_cast<unsigned long>(o2ValveTime);
    this->timeBeforeClosingCO2Valve = millis() + static_cast<unsigned long>(co2ValveTime);
    this->timeBeforeClosingAirValve = millis() + static_cast<unsigned long>(airValveTime);

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
 * @param Valve The valve to be closed.
 * @param error The error value.
 * @return The time before closing the valve in milliseconds.
 * @note This function makes a few simplifications that must be tested :
 * - The flow rate is assumed to be constant even if opening the valve will increase the pressure in the chamber.
 * - The equation assumes that no gas is lost when new gas is added to the chamber, wich is false.
 *   Because we will have multiple adjustment in a row, I assume that the error will disapear after a few iterations.
 */
float PressureChamberController::calculateTimeBeforeClosingValve(eValves Valve, float error)
{
    float timeBeforeClosing = 0;
    float q = 0.0f;           // Flow rate in liters per second
    float volumeToAdd = 0.0f; // Volume to add in liters
    switch (Valve)
    {
    case O2:
        q = (PI * pow(R, 4)) / (8 * MU_O2 * L) * (P_APPROV - P_CHAMBER); // Flow rate equation for O2 (Poiseuille's law)
        volumeToAdd = error * PERCENT_TO_LITERS;                         // Volume to add in liters
        timeBeforeClosing = volumeToAdd / q * SECONDS_TO_MILLIS;         // Time in milliseconds (t=V/Q)
        break;
    case CO2:
        q = (PI * pow(R, 4)) / (8 * MU_CO2 * L) * (P_APPROV - P_CHAMBER); // Flow rate equation for CO2 (Poiseuille's law)
        volumeToAdd = error * PPM_TO_LITERS;                              // Volume to add in liters
        timeBeforeClosing = volumeToAdd / q * SECONDS_TO_MILLIS;          // Time in milliseconds (t=V/Q)
        break;
    case AIR:
        q = (PI * pow(R, 4)) / (8 * MU_AIR * L) * (P_APPROV - P_CHAMBER); // Flow rate equation for air (Poiseuille's law)
        volumeToAdd = error * PERCENT_TO_LITERS;                          // Volume to add in liters
        timeBeforeClosing = volumeToAdd / q * SECONDS_TO_MILLIS;          // Time in milliseconds (t=V/Q)
        break;
    default:
        break;
    }
    return timeBeforeClosing;
}

/**
 * @brief Returns the state of the specified valve.
 * @param Valve The valve to check.
 * @return The state of the valve (true for open, false for closed).
 */
bool PressureChamberController::getValveState(eValves Valve) const
{
    if (!this->pressureChamberState)
        return false;

    switch (Valve)
    {
    case O2:
        return (timeBeforeClosingO2Valve > millis()) ? true : false;
    case CO2:
        bool status = (co2Sensor.getStatus() == eGMP251Status::GMP_251_STATUS_OK) || (co2Sensor.getStatus() == eGMP251Status::GMP_251_STATUS_INITIALIZED);
        return ((timeBeforeClosingCO2Valve > millis()) && status) ? true : false;
    case AIR:
        return (timeBeforeClosingAirValve > millis()) ? true : false;
    case SAFETY:
        return false;
    default:
        return false;
    }
}

/**
 * @brief Sets the reference level for the specified valve.
 * @param Valve The valve to set the reference level for.
 * @param ReferenceLevel The reference level to set.
 */
void PressureChamberController::setReferenceLevel(eValves Valve, float ReferenceLevel)
{
    switch (Valve)
    {
    case O2:
        this->o2Ref = ReferenceLevel;
        this->o2MinRef = ReferenceLevel - O2_DEAD_ZONE;
        this->o2MaxRef = ReferenceLevel + O2_DEAD_ZONE;
        break;
    case CO2:
        this->co2Ref = ReferenceLevel;
        this->co2MinRef = ReferenceLevel - CO2_DEAD_ZONE;
        this->co2MaxRef = ReferenceLevel + CO2_DEAD_ZONE;
        break;
    default:
        break;
    }
}