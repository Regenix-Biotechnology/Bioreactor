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

    eMotorStatus setSpeed(float speed);
    eMotorStatus stop();

private:
    static constexpr uint8_t CONFIG_MSG_SIZE = 7;

    static constexpr uint32_t RUNNING_TORQUE = 0x8F82; // Torque of the motor while running (half of the default torque)

    static const uint8_t MOTOR_DRV_IHOLD_IRUN_ADDR[MOTOR_NAME_MAX];
    static const uint8_t MOTOR_DRV_AMAX_ADDR[MOTOR_NAME_MAX];
    static const uint8_t MOTOR_DRV_VMAX_ADDR[MOTOR_NAME_MAX];
    static const uint8_t MOTOR_DRV_SET_SPEED_ADDR[MOTOR_NAME_MAX];
    static const uint8_t MOTOR_DRV_SET_MODE_ADDR[MOTOR_NAME_MAX];
    static const uint8_t SET_SPEED_CONFIG_MSG_ADDR_LIST[MOTOR_NAME_MAX][CONFIG_MSG_SIZE];
    static const uint32_t SET_SPEED_CONFIG_MSG_DATA_LIST[CONFIG_MSG_SIZE];

    static constexpr float FREQ_CLOCK = 13.3 * (10 ^ 6);                                                                                                     // Hz
    static constexpr float DEFREE_PER_STEP = 1.8;                                                                                                            // datasheet kamoer
    static constexpr float ML_PER_RPM = 0.1388;                                                                                                              // gros approx datasheet kamoer
    static constexpr uint16_t MICRO_STEP_PER_STEP = 256;                                                                                                     // datasheet p.30
    static constexpr float REG_TO_ML_PER_MIN = (FREQ_CLOCK / 2 / (2 ^ 23) / MICRO_STEP_PER_STEP / (360 / DEFREE_PER_STEP)) /*rotation/s*/ * 60 * ML_PER_RPM; // conversion formula p. 52
    static constexpr float ML_PER_MIN_TO_REG = 1 / REG_TO_ML_PER_MIN;

    DriveTmc5041 *_drive_handle;
    eMotorName _motorName;
    bool _isInit;
};

#endif // STEPPER_MOTOR_H