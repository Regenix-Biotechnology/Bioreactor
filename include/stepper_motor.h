#ifndef STEPPER_MOTOR_H
#define STEPPER_MOTOR_H

#include <Arduino.h>
#include "tmc5041.h"

typedef enum
{
    MOTOR_1 = 0,
    MOTOR_2,
    MOTOR_NAME_MAX
} eMotorName;

/**
 * @brief Class to control the individual motors from a TMC5041 drive using a DriveTmc5041 object
 * that needs to be previously created and passed to the StepperMotor controller
 *
 * Link to the datasheet:
 *  https://www.analog.com/media/en/technical-documentation/data-sheets/TMC5041_datasheet_rev1.16.pdf
 */
class StepperMotor
{
public:
    StepperMotor(DriveTmc5041 *drive_handle, eMotorName motorName);

    eMotorStatus begin();
    eMotorStatus begin128MS();
    eMotorStatus setSpeed(float speed, uint16_t microStep);
    eMotorStatus stop();
    void setPumpSpeed(float speed, uint16_t microStep);
    void setAMAX(uint32_t amax);

private:
    static constexpr uint8_t CONFIG_MSG_SIZE = 15;
    static constexpr uint32_t RUNNING_TORQUE = 0x00060F06; // 0x8F82; // Torque of the motor while running (half of the default torque) //1 1F04  0x00060F08

    float targetPumpSpeed;
    uint16_t microstep;

    // Variables membres à ajouter à votre classe / contexte
    uint32_t _rampStartTime = 0;
    bool _isRamping = false;
    const uint32_t RAMP_DURATION_MS = 120000; // 2 minutes = 120 000 ms
    float _startSpeed = 0.0f;                 // Vitesse d'origine au moment où le ramp-up commence
    float _lastTargetSpeed = 0.0f;
    float _currentCalculatedSpeed = 0.0f; // Vitesse courante calculée
    float _lastRequestedTarget = 0.0f;    // Garde en mémoire la consigne demandée

    float speed = 0.0f;

    static const uint8_t MOTOR_DRV_IHOLD_IRUN_ADDR[MOTOR_NAME_MAX];
    static const uint8_t MOTOR_DRV_AMAX_ADDR[MOTOR_NAME_MAX];
    static const uint8_t MOTOR_DRV_VMAX_ADDR[MOTOR_NAME_MAX];
    static const uint8_t MOTOR_DRV_SET_SPEED_ADDR[MOTOR_NAME_MAX];
    static const uint8_t MOTOR_DRV_SET_MODE_ADDR[MOTOR_NAME_MAX];
    static const uint8_t SET_SPEED_CONFIG_MSG_ADDR_LIST[MOTOR_NAME_MAX][CONFIG_MSG_SIZE];
    static const uint32_t SET_SPEED_CONFIG_MSG_DATA_LIST[CONFIG_MSG_SIZE];
    static const uint32_t SET_SPEED_CONFIG_MSG_DATA_LIST_32[CONFIG_MSG_SIZE];

    static constexpr float FREQ_CLOCK = 13200000.0f;     // 13.3 * (10 ^ 6);                                                                                                           // Hz
    static constexpr float DEFREE_PER_STEP = 1.8;        // datasheet kamoer
    static constexpr float ML_PER_RPM = 0.1388f;         // gros approx datasheet kamoer
    static constexpr uint16_t MICRO_STEP_PER_STEP = 256; // datasheet p.30

    // 1. Number of micro step (ex: 200 * 256 = 51200)
    static constexpr float MICROSTEPS_PER_REV = (360.0f / DEFREE_PER_STEP) * MICRO_STEP_PER_STEP;
    // 2. Conversion factor mL/min ->  Register VMAX (CORRECTED)
    static constexpr float ML_PER_MIN_TO_REG = (MICROSTEPS_PER_REV / (60.0f * ML_PER_RPM)) * (8388608.0f / (FREQ_CLOCK * 2.0f));
    // 3. Inversion
    static constexpr float REG_TO_ML_PER_MIN = 1.0f / ML_PER_MIN_TO_REG;

    DriveTmc5041 *_drive_handle;
    eMotorName _motorName;
    bool _isInit;
};

#endif // STEPPER_MOTOR_H