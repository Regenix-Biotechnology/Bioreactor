#ifndef PRESSURE_CHAMBER_CONTROLLER_H
#define PRESSURE_CHAMBER_CONTROLLER_H

#include <Arduino.h>

typedef enum
{
    O2 = 0,
    CO2,
    AIR,
    SAFETY,
    MAX_VALVES
} eValves;

/**
 * @class TemperatureController
 * @brief Implements the control loop for the pressure chamber (O2, CO2, air)
 */
class PressureChamberController
{
public:
    PressureChamberController();
    void update(float o2Concentration, float co2Concentration, float pressure);
    bool getValveState(eValves Valve) const;
    void setReferenceLevel(eValves Valve, float ReferenceLevel);
    void setPressureChamberState(bool state) { this->pressureChamberState = state; }

private:
    float calculateTimeBeforeClosingValve(eValves Valve, float error);

    // Control loop parameters.
    bool pressureChamberState;
    unsigned long timeBeforeClosingO2Valve;
    unsigned long timeBeforeClosingCO2Valve;
    unsigned long timeBeforeClosingAirValve;

    // Reference values.
    float o2MinRef;
    float o2MaxRef;
    float co2MinRef;
    float co2MaxRef;
    float co2Ref;
    float o2Ref;

    // Control outputs.
    bool o2ValveState;
    bool co2ValveState;
    bool airValveState;
    bool safetyValveState;

    // Flow equations for the valves (Poiseuille's law).
    static constexpr float R = 0.0043;                           // Radius of the inner diameter of the tube
    static constexpr float L = 2;                                // Length of the tube
    static constexpr float MU_AIR = 0.000018;                    // Viscosity of the fluid (air)
    static constexpr float MU_O2 = 0.00002;                      // Viscosity of the fluid (O2)
    static constexpr float MU_CO2 = 0.000015;                    // Viscosity of the fluid (CO2)
    static constexpr float P_APPROV = 30 * 6895;                 // Pressure of the O2 and CO2 tanks (Pa)
    static constexpr float P_CHAMBER = 25 * 6895;                // Pressure of the pressure chamber (Pa)
    static constexpr float P_CHAMBER_MIN = P_CHAMBER - 2 * 6895; // Minimum pressure in the chamber (Pa) is 2 psi less than the setpoint
    static constexpr float V = 1.296;                            // Volume of the pressure chamber (L)

    // Constants for the control loop.
    static constexpr float O2_REF = 85.0f;
    static constexpr float CO2_REF = 50000.0f;                      // ppm
    static constexpr float CO2_DEAD_ZONE = 100.0f;                  // ppm
    static constexpr float O2_DEAD_ZONE = 0.1f;                     // % O2
    static constexpr float PERCENT_TO_LITERS = 0.01 * V;            // Convert percentage to liters
    static constexpr float PPM_TO_LITERS = 0.000001 * V;            // Convert ppm to liters
    static constexpr float SECONDS_TO_MILLIS = 1000.0f;             // Convert seconds to milliseconds
    static constexpr float CO2_DISPLACEMENT_RATIO = 0.2f;           // Empirical factor to tune
    static constexpr float AIR_VALVE_OPEN_TIME = 1000.0f;           // Time to open the air valve (ms)
    static constexpr float CORRECTION_FACTOR_O2 = 0.5f;             // Correction factor for O2
    static constexpr float CORRECTION_FACTOR_O2_REDUCTION = 40.0f;  // Correction factor for O2 reduction
    static constexpr float CORRECTION_FACTOR_CO2 = 1.0f;            // Correction factor for CO2
    static constexpr float CORRECTION_FACTOR_CO2_REDUCTION = 30.0f; // Correction factor for CO2 reduction
    static constexpr float CORRECTION_FACTOR_O2_SMALL = CORRECTION_FACTOR_O2 * 3.0f;
    static constexpr float CORRECTION_FACTOR_CO2_SMALL = CORRECTION_FACTOR_CO2 * 4.0f;
    static constexpr float SMALL_O2_ERROR_THRESHOLD = 10.0f;     // 5% O2
    static constexpr float SMALL_CO2_ERROR_THRESHOLD = 15000.0f; // 5000 ppm CO2
};

#endif // PRESSURE_CHAMBER_CONTROLLER_H
