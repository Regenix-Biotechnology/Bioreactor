#include "stepper_motor.h"

const uint8_t StepperMotor::MOTOR_DRV_IHOLD_IRUN_ADDR[MOTOR_NAME_MAX] = {0x30, 0x50};
const uint8_t StepperMotor::MOTOR_DRV_AMAX_ADDR[MOTOR_NAME_MAX] = {0x26, 0x46};
const uint8_t StepperMotor::MOTOR_DRV_SET_SPEED_ADDR[MOTOR_NAME_MAX] = {0x27, 0x47};
const uint8_t StepperMotor::MOTOR_DRV_SET_MODE_ADDR[MOTOR_NAME_MAX] = {0x20, 0x40};
const uint8_t StepperMotor::SET_SPEED_CONFIG_MSG_ADDR_LIST[MOTOR_NAME_MAX][CONFIG_MSG_SIZE] = {{0x6C, 0x30, 0x2C, 0x10, 0x32, 0x31, 0x26}, {0x7C, 0x50, 0x4C, 0x18, 0x52, 0x51, 0x46}};
const uint32_t StepperMotor::SET_SPEED_CONFIG_MSG_DATA_LIST[CONFIG_MSG_SIZE] = {0x010100C5, RUNNING_TORQUE * 2, 0x00002710, 0x003501C8, 0x00061A80, 0x00007530, 0x00001388};

/**
 * @brief Construct a new StepperMotor object
 *
 * @param drive_handle TMC5041 drive object
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
 *
 * @param speed speed in ml/min (+ is clockwise, - is counterclockwise)
 * @return eMotorStatus MOTOR_STATUS_OK if no problem occured else return error code
 */
eMotorStatus StepperMotor::setSpeed(float speed)
{
    if (!_isInit)
        return MOTOR_STATUS_NOT_INITIALISED;

    eMotorMode direction = MOTOR_MODE_SPEED_CONTROL_COUNTERCLOCKWISE;
    uint32_t torque = RUNNING_TORQUE;
    if (speed == 0.0)
    {
        torque = 0;
    }
    else if (speed < 0.0)
    {
        direction = MOTOR_MODE_SPEED_CONTROL_CLOCKWISE;
        speed = fabsf(speed);
    }

    _drive_handle->tmc_write(MOTOR_DRV_IHOLD_IRUN_ADDR[_motorName], torque);
    _drive_handle->tmc_write(MOTOR_DRV_SET_SPEED_ADDR[_motorName], uint32_t(speed * ML_PER_MIN_TO_REG));
    _drive_handle->tmc_write(MOTOR_DRV_SET_MODE_ADDR[_motorName], direction);

    return MOTOR_STATUS_OK;
}

/**
 * @brief Set motor speed to zero and make the motor be a freewheel
 *
 * @return eMotorStatus MOTOR_STATUS_OK if no problem occured else return error code
 */
eMotorStatus StepperMotor::stop()
{
    if (!_isInit)
        return MOTOR_STATUS_NOT_INITIALISED;

    this->setSpeed(0);

    return MOTOR_STATUS_OK;
}
