#include "pressure_chamber_controller.h"

/**
 * @brief Constructor to initialize the control loop parameters.
 */
PressureChamberController::PressureChamberController()
    : o2MinRef(O2_MIN_REF),
      o2MaxRef(O2_MAX_REF),
      co2MinRef(CO2_REF - CO2_DEAD_ZONE),
      co2MaxRef(CO2_REF + CO2_DEAD_ZONE),
      co2Ref(CO2_REF),
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

    unsigned long o2ValveTime = 0;
    unsigned long co2ValveTime = 0;

    // --- O₂ Control Logic ---
    if (o2Concentration < this->o2MinRef)
    {
        float o2Error = this->o2MaxRef - o2Concentration;
        o2ValveTime = calculateTimeBeforeClosingValve(O2, o2Error);
    }

    // --- CO₂ Control Logic ---
    if (co2Concentration > this->co2MaxRef)
    {
        float co2Error = co2Concentration - this->co2Ref;
        float effectiveO2Error = co2Error / co2Concentration;
        unsigned long o2ValveTimeFromCo2 = calculateTimeBeforeClosingValve(O2, effectiveO2Error);
        o2ValveTime = max(o2ValveTime, o2ValveTimeFromCo2);
    }
    else if (co2Concentration < this->co2MinRef)
    {
        float co2Error = this->co2Ref - co2Concentration;
        co2ValveTime = calculateTimeBeforeClosingValve(CO2, co2Error);
    }

    // --- Pressure Control Logic ---
    if (pressure < P_CHAMBER_MIN)
    {
        unsigned long o2ValveTimeFromPressure = millis() + AIR_VALVE_OPEN_TIME;
        o2ValveTime = max(o2ValveTime, o2ValveTimeFromPressure);
    }

    // Apply the calculated times
    this->timeBeforeClosingO2Valve = o2ValveTime;
    this->timeBeforeClosingCO2Valve = co2ValveTime;

    // --- DEBUG OUTPUT ---
    Serial.println(">O2 Opening Time: " + String((timeBeforeClosingO2Valve - millis())));
    Serial.println(">CO2 Opening Time: " + String((timeBeforeClosingCO2Valve - millis())));
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
unsigned long PressureChamberController::calculateTimeBeforeClosingValve(eValves Valve, float error)
{
    unsigned long timeBeforeClosing = 0;
    float q = 0.0f;           // Flow rate in liters per second
    float volumeToAdd = 0.0f; // Volume to add in liters
    switch (Valve)
    {
    case O2:
        q = (PI * pow(R, 4)) / (8 * MU_O2 * L) * (P_APPROV - P_CHAMBER);                                // Flow rate equation for O2 (Poiseuille's law)
        volumeToAdd = error * PERCENT_TO_LITERS;                                                        // Volume to add in liters
        timeBeforeClosing = millis() + static_cast<unsigned long>(volumeToAdd / q * SECONDS_TO_MILLIS); // Time in milliseconds (t=V/Q)
        break;
    case CO2:
        q = (PI * pow(R, 4)) / (8 * MU_CO2 * L) * (P_APPROV - P_CHAMBER);                               // Flow rate equation for CO2 (Poiseuille's law)
        volumeToAdd = error * PPM_TO_LITERS;                                                            // Volume to add in liters
        timeBeforeClosing = millis() + static_cast<unsigned long>(volumeToAdd / q * SECONDS_TO_MILLIS); // Time in milliseconds (t=V/Q)
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
        return (timeBeforeClosingCO2Valve > millis()) ? true : false;
    case AIR:
        return false;
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
        this->o2MinRef = ReferenceLevel;
        this->o2MaxRef = O2_MAX_REF; // O2 max reference is fixed to 95%
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