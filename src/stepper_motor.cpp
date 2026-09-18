#include "stepper_motor.h"

const uint8_t StepperMotor::MOTOR_DRV_IHOLD_IRUN_ADDR[MOTOR_NAME_MAX] = {0x30, 0x50};
const uint8_t StepperMotor::MOTOR_DRV_AMAX_ADDR[MOTOR_NAME_MAX] = {0x26, 0x46};
const uint8_t StepperMotor::MOTOR_DRV_SET_SPEED_ADDR[MOTOR_NAME_MAX] = {0x27, 0x47};
const uint8_t StepperMotor::MOTOR_DRV_SET_MODE_ADDR[MOTOR_NAME_MAX] = {0x20, 0x40};
const uint8_t StepperMotor::SET_SPEED_CONFIG_MSG_ADDR_LIST[MOTOR_NAME_MAX][CONFIG_MSG_SIZE] = {
    {0x6C, 0x30, 0x2C, 0x10, 0x32, 0x13, 0x26, 0x20, 0x23, 0x24, 0x25, 0x27, 0x28, 0x2A, 0x2B}, // Moteur 1
    {0x7C, 0x50, 0x4C, 0x18, 0x52, 0x18, 0x46, 0x40, 0x43, 0x44, 0x45, 0x47, 0x48, 0x4A, 0x4B}  // Moteur 2
};

const uint32_t StepperMotor::SET_SPEED_CONFIG_MSG_DATA_LIST[CONFIG_MSG_SIZE] = {
    0x11010145, // [0]  0x6C : CHOPCONF
    0x00060F08, // [1]  0x30 : IHOLD_IRUN
    0x00002710, // [2]  0x2C : TZEROWAIT
    0x00000000, // [3]  0x10 : GCONF
    0x00000006, // [4]  0x32 : TPOWERDOWN = 10 (comme driver.TPOWERDOWN(10))
    0x000001F4, // [5]  0x13 : TPWMTHRS = 500 (comme driver.TPWMTHRS(500))
    0x0000000A, // [6]  0x26 : AMAX
    0x00000000, // [7]  0x20 : RAMPMODE
    0x00000400, // [8]  0x23 : VSTART
    0x00000002, // [9]  0x24 : A1
    0x00120000, // [10] 0x25 : V1
    0x00043E00, // [11] 0x27 : VMAX
    0x00000100, // [12] 0x28 : DMAX
    0x00000002, // [13] 0x2A : D1
    0x00000030  // [14] 0x2B : VSTOP
};

/**
 * @brief Construct a new StepperMotor object
 * @param drive_handle TMC5041 drive
 * @param motorName eMotorName to use for this motor name
 */
StepperMotor::StepperMotor(DriveTmc5041 *drive_handle, eMotorName motorName)
    : _drive_handle(drive_handle),
      _motorName(motorName),
      _isInit(false)
{
}

/**
 * @brief Initialise the motor specific control
 * @return eMotorStatus MOTOR_STATUS_OK if no problem occured else return error code
 * @warning The TMC5041 object provided should already be begined before calling this
 */
eMotorStatus StepperMotor::begin()
{
    if (!_drive_handle)
        return MOTOR_STATUS_NULL_VARIABLE;
    if (_motorName >= MOTOR_NAME_MAX)
        return MOTOR_STATUS_INCORRECT_VARIABLE;

    // Motor specific configuration
    for (uint8_t i = 0; i < CONFIG_MSG_SIZE; i++)
    {
        _drive_handle->tmc_write(SET_SPEED_CONFIG_MSG_ADDR_LIST[_motorName][i], SET_SPEED_CONFIG_MSG_DATA_LIST[i]);
    }
    _isInit = true;
    return MOTOR_STATUS_OK;
}

/**
 * @brief Set the motor speed in ml/min
 * @param speed speed in ml/min (+ is clockwise, - is counterclockwise)
 * @param microStep number of micro-steps use by the motor (ex: 16, 32, 64, 128, 256)
 * @return eMotorStatus MOTOR_STATUS_OK if no problem occured else return error code
 */
eMotorStatus StepperMotor::setSpeed(float speed, uint16_t microStep)
{
    speed = speed / (256.0f / static_cast<float>(microStep));
    if (!_isInit)
        return MOTOR_STATUS_NOT_INITIALISED;

    eMotorMode direction = MOTOR_MODE_SPEED_CONTROL_COUNTERCLOCKWISE;
    uint32_t torque = 0x00060F06;
    if (speed == 0.0)
    {
    }
    else if (speed < 0.0)
    {
        direction = MOTOR_MODE_SPEED_CONTROL_CLOCKWISE;
        speed = fabsf(speed);
    }
    else if (speed > 0.0)
    {
        direction = MOTOR_MODE_SPEED_CONTROL_COUNTERCLOCKWISE;
        speed = fabsf(speed);
    }
    _drive_handle->tmc_write(MOTOR_DRV_IHOLD_IRUN_ADDR[_motorName], torque);
    _drive_handle->tmc_write(MOTOR_DRV_SET_SPEED_ADDR[_motorName], uint32_t(speed * ML_PER_MIN_TO_REG));
    _drive_handle->tmc_write(MOTOR_DRV_SET_MODE_ADDR[_motorName], direction);
    return MOTOR_STATUS_OK;
}

/**
 * @brief Set motor speed to zero and make the motor be a freewheel
 * @return eMotorStatus MOTOR_STATUS_OK if no problem occured else return error code
 */
eMotorStatus StepperMotor::stop()
{
    if (!_isInit)
        return MOTOR_STATUS_NOT_INITIALISED;
    this->setSpeed(0, 256);
    return MOTOR_STATUS_OK;
}

/**
 * @brief Set the motor AMAX value (acceleration)
 * This function allows to set the acceleration of the motor by writing the AMAX register
 * @param amax acceleration value
 */
void StepperMotor::setAMAX(uint32_t amax)
{
    if (!_isInit)
        return;

    _drive_handle->tmc_write(MOTOR_DRV_AMAX_ADDR[_motorName], amax);
}
