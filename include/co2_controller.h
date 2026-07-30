#ifndef CO2_CONTROLLER_H
#define CO2_CONTROLLER_H

#include <Arduino.h>

#define ARDUINO_I2C_ADDR 0x10

class CO2Controller
{
public:
    CO2Controller();
    float calculateTimeBeforeClosingValve(float error);
    void setReferenceLevel(float ReferenceLevel);
    void update(float co2Concentration);
    float GetCO2Value();
    bool getValveState();
    void regulation();

private:
    float co2MinRef;
    float co2MaxRef;
    float co2Ref;
    bool isCO2ValveOpen = false; // Stocke l'état d'hystérésis

    // Flow equations for the valves (Poiseuille's law).
    static constexpr float R = 0.0043;        // Radius of the inner diameter of the tube
    static constexpr float L = 2;             // Length of the tube
    static constexpr float MU_AIR = 0.000018; // Viscosity of the fluid (air)
    static constexpr float MU_O2 = 0.00002;   // Viscosity of the fluid (O2)
    static constexpr float MU_CO2 = 0.000015; // Viscosity of the fluid (CO2)
    static constexpr float V = 144;           // Volume of the pressure chamber (L)
    // Constants for the control loop.
    static constexpr float CO2_REF = 50000.0f;   // ppm
    static constexpr float CO2_DEAD_ZONE = 0.1f; // %

    static constexpr float PERCENT_TO_LITERS = 0.01 * V;  // Convert percentage to liters
    static constexpr float PPM_TO_LITERS = 0.000001 * V;  // Convert ppm to liters
    static constexpr float SECONDS_TO_MILLIS = 1000.0f;   // Convert seconds to milliseconds
    static constexpr float CO2_DISPLACEMENT_RATIO = 0.2f; // Empirical factor to tune
    static constexpr float AIR_VALVE_OPEN_TIME = 1000.0f; // Time to open the air valve (ms)

    static constexpr float CORRECTION_FACTOR_CO2 = 1.0f;            // Correction factor for CO2
    static constexpr float CORRECTION_FACTOR_CO2_REDUCTION = 30.0f; // Correction factor for CO2 reduction
    static constexpr float CORRECTION_FACTOR_CO2_SMALL = CORRECTION_FACTOR_CO2 * 4.0f;
    static constexpr float SMALL_CO2_ERROR_THRESHOLD = 15000.0f; // 5000 ppm CO2

    static constexpr float P_B_APPROV = 30 * 6895; // Pressure of the O2 and CO2 tanks (Pa)
    static constexpr float P_B_CHAMBER = 25 * 6895;
};

// void CO2Value(float co2Value);

#endif