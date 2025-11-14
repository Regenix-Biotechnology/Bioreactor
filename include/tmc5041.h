#ifndef TMC5041_H
#define TMC5041_H

#include <Arduino.h>
#include <SPI.h>

typedef enum
{
    MOTOR_STATUS_OK = 0,
    MOTOR_STATUS_NOT_INITIALISED,
    MOTOR_STATUS_INCORRECT_VARIABLE,
    MOTOR_STATUS_NULL_VARIABLE,

    MOTOR_STATUS_MAX
} eMotorStatus;

typedef enum
{
    MOTOR_MODE_POSITION_CONTROL = 0x0,
    MOTOR_MODE_SPEED_CONTROL_CLOCKWISE = 0x1,
    MOTOR_MODE_SPEED_CONTROL_COUNTERCLOCKWISE = 0x2,
    MOTOR_MODE_HOLD = 0x3,

    MOTOR_MODE_MAX
} eMotorMode;

/**
 * @brief Low level class for the TMC5041 driver
 *
 * Link to the datasheet:
 * https://www.analog.com/media/en/technical-documentation/data-sheets/TMC5041_datasheet_rev1.16.pdf
 */
class DriveTmc5041
{
public:
    DriveTmc5041(SPIClass *spi_handle, uint8_t cs);

    eMotorStatus begin();

    void tmc_write(uint8_t address, uint32_t data);
    uint32_t tmc_read(uint8_t address);

private:
    SPIClass *_spi;
    uint8_t _cs;
    bool _isInit;
};

#endif // MOTOR_DRIVE_TMC_H