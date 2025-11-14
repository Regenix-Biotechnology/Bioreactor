#include "tmc5041.h"

/**
 * @brief Construct a new DriveTmc5041 object
 *
 * @param spi_handle SPI object to use
 * @param cs Chip select pin for this driver
 */
DriveTmc5041::DriveTmc5041(SPIClass *spi_handle, uint8_t cs)
    : _spi(spi_handle),
      _cs(cs),
      _isInit(false)
{
}

/**
 * @brief Initialize the TMC5041 driver (SPI should be initialised before this)
 *
 * @return eMotorStatus MOTOR_STATUS_OK if no problem occured else return error code
 */
eMotorStatus DriveTmc5041::begin()
{
  pinMode(_cs, OUTPUT);
  if (!_spi)
    return MOTOR_STATUS_NULL_VARIABLE;

  // send global drive config
  tmc_write(0x00, (uint32_t)0x08);

  _isInit = true;
  return MOTOR_STATUS_OK;
}

/**
 * @brief Function to send message to the TMC5041 drive
 *
 * @param address Register to send data to
 * @param data data to send to driver
 */
void DriveTmc5041::tmc_write(uint8_t address, uint32_t data)
{
  digitalWrite(_cs, LOW);
  SPI.transfer(address | 0b10000000); // Write bit
  SPI.transfer((data >> 24) & 0xFF);
  SPI.transfer((data >> 16) & 0xFF);
  SPI.transfer((data >> 8) & 0xFF);
  SPI.transfer(data & 0xFF);
  digitalWrite(_cs, HIGH);
}

/**
 * @brief Function to read register from the TMC5041 drive
 *
 * @param address Register to read
 * @return uint32_t the read data received from the driver
 */
uint32_t DriveTmc5041::tmc_read(uint8_t address)
{
  uint32_t value = 0;
  digitalWrite(_cs, LOW);
  SPI.transfer(address & 0x7F);

  for (int i = 0; i < 4; i++)
    value = (value << 8) | SPI.transfer(0x00);
  digitalWrite(_cs, HIGH);
  return value;
}