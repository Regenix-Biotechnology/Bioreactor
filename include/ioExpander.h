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
 * The output registers are only written on the I²C bus when an output changes, when the last
 * write failed, or every REFRESH_INTERVAL_MS (periodic refresh). This keeps the shared I²C bus
 * free for the sensors while still correcting any missed write.
 *
 * @see Datasheet: https://www.diodes.com/assets/Datasheets/PI4IOE5V6524.pdf
 */
class IOExpander
{
public:
    explicit IOExpander(TwoWire *pWire = &Wire);

    bool begin();

    bool setEfuse(uint8_t channel, bool outputState);

    /** @brief true if the last write to the expander succeeded. */
    bool isHealthy() const { return !_dirty; }

    // --- Constants ---
    static constexpr uint8_t OUTPUT_COUNT = 24;         ///< 20 eFuses + 4 debug LEDs
    static constexpr uint8_t REGISTER_OFF = 0x00;       ///< Default inactive state (logic LOW)
    static constexpr uint8_t CONFIG_OUTPUT_MODE = 0x00; ///< Configure all IOs as outputs

private:
    TwoWire *_pWire;

    // --- Internal output registers mirror ---
    static constexpr uint8_t IOE_PORT_BYTES = 3;
    uint8_t _outputs[IOE_PORT_BYTES] = {REGISTER_OFF, REGISTER_OFF, REGISTER_OFF};

    // --- Write management ---
    static constexpr unsigned long REFRESH_INTERVAL_MS = 5000UL; ///< periodic rewrite of all outputs
    static constexpr unsigned long RETRY_INTERVAL_MS = 100UL;    ///< min delay between attempts after a failure
    bool _dirty = true;                                          ///< registers on the chip may differ from _outputs
    unsigned long _lastWriteMs = 0;                              ///< last successful write
    unsigned long _lastAttemptMs = 0;                            ///< last write attempt

    // --- PI4IOE5V6524 register addresses ---
    static constexpr uint8_t IOE_I2C_ADDRESS = (0x46 >> 1); ///< 7-bit I²C address
    static constexpr uint8_t IOE_REG_INPUT = 0x00;
    static constexpr uint8_t IOE_REG_OUTPUT = 0x04;
    static constexpr uint8_t IOE_REG_POLARITY = 0x08;
    static constexpr uint8_t IOE_REG_CONFIG = 0x0C;

    bool writeBytes(uint8_t reg, const uint8_t *data, size_t len) const;
    bool writeOutputs();
};

#endif