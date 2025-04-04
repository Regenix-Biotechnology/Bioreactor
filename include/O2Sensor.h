#ifndef O2SENSOR_H
#define O2SENSOR_H

#include <Arduino.h>
#include <Wire.h>

typedef enum
{
  O2_SENSOR_STATUS_OK = 0,
  O2_SENSOR_STATUS_NOT_INITIALIZED,
  O2_SENSOR_STATUS_INITIALIZED,
  O2_SENSOR_STATUS_FAILED_TO_SEND_REQUEST,
  O2_SENSOR_STATUS_INVALID_RESPONSE,
  O2_SENSOR_STATUS_TIMEOUT_EXCEEDED,

  O2_SENSOR_STATUS_MAX
} eO2SensorStatus;

/**
 * @brief DFRobot O2 Sensor Driver (I2C Communication).
 * @details Communicates with the DFRobot O2 sensor using I2C protocol.
 * @link https://www.dfrobot.com/product-2569.html?srsltid=AfmBOop59t9vDFFckZv9SZst7JjIIR8pOw8MOQoda4LARB1Vcf8aH6wk
 */
class O2Sensor
{
public:
  O2Sensor(TwoWire *pWire = &Wire);
  ~O2Sensor() {}

  eO2SensorStatus begin();
  float getO2();
  uint8_t readCalibrationState();
  bool calibration_20_9();
  bool calibration_99_5();
  bool clearCalibration();
  eO2SensorStatus getStatus() { return status; }

private:
  eO2SensorStatus readData(uint8_t reg, uint8_t *data, uint8_t len);
  eO2SensorStatus writeData(uint8_t reg, uint8_t *data, uint8_t len);

  TwoWire *_pWire;
  eO2SensorStatus status;

  static constexpr uint8_t CALIBRATION_20_9 = 0x01;
  static constexpr uint8_t CALIBRATION_99_5 = 0x02;
  static constexpr uint8_t CALIBRATION_CLEAR = 0x03;
  static constexpr uint8_t OXYGEN_DATA = 0x10;
  static constexpr uint8_t CALIBRATION_STATE = 0x13;
  static constexpr uint8_t CALIBRATION_SENSOR = 0x18;
  static constexpr uint8_t I2C_ADDRESS = 0x70;
  static constexpr uint8_t DATA_BUFFER_SIZE = 3;
  static constexpr uint8_t DECIMAL_PLACE_1 = 10;
  static constexpr uint8_t DECIMAL_PLACE_2 = 100;
};

#endif