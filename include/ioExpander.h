#ifndef io_EXPANDER_H
#define io_EXPANDER_H
#include <Arduino.h>
#include <Wire.h>

// Be careful to place the IO Expander initialization first in the logic
// to ensure the loads are not activated unintentionally.

/**
 * @class IOExpander
 * @brief Interface class for controlling eFuses and debug LEDs through
 *        the PI4IOE5V6524 I²C I/O Expander
 *
 * @see Datasheet: https://www.diodes.com/assets/Datasheets/PI4IOE5V6524.pdf
 */
class IOExpander
{
public:
    explicit IOExpander(TwoWire *pWire = &Wire);

    bool begin();

    void setEfuse(uint8_t channel, bool outputState);

    // --- Constants ---
    static constexpr uint8_t OUTPUT_COUNT = 24;         ///< 20 eFuses + 4 debug LEDs
    static constexpr uint8_t REGISTER_OFF = 0x00;       ///< Default inactive state (logic LOW)
    static constexpr uint8_t CONFIG_OUTPUT_MODE = 0x00; ///< Configure all IOs as outputs

private:
    TwoWire *_pWire;

    // --- Internal output registers mirror ---
    static constexpr uint8_t IOE_PORT_BYTES = 3;
    uint8_t _outputs[IOE_PORT_BYTES] = {REGISTER_OFF, REGISTER_OFF, REGISTER_OFF};

    // --- PI4IOE5V6524 register addresses ---
    static constexpr uint8_t IOE_I2C_ADDRESS = (0x46 >> 1); ///< 7-bit I²C address
    static constexpr uint8_t IOE_REG_INPUT = 0x00;
    static constexpr uint8_t IOE_REG_OUTPUT = 0x04;
    static constexpr uint8_t IOE_REG_POLARITY = 0x08;
    static constexpr uint8_t IOE_REG_CONFIG = 0x0C;

    bool writeBytes(uint8_t reg, const uint8_t *data, size_t len) const;
};

#endif