#include "ioExpander.h"

 // --- Constructor ---
/**
 * @brief Construct a new IOExpander object
 * @param pWire Pointer to the I²C interface (SDA/SCL)
 */
IOExpander::IOExpander(TwoWire *pWire)
    : _pWire(pWire)
{
}

/**
 * @brief Send I²C commands to the IO Expander
 * 
 * @param reg Register address to write to
 * @param data Pointer to the data buffer to send
 * @param len Number of bytes to transmit
 * @return true if transmission is successful
 * @return false if transmission fails
 */
bool IOExpander::writeBytes(uint8_t reg, const uint8_t *data, size_t len) const
{
    _pWire->beginTransmission(IOE_I2C_ADDRESS);
    _pWire->write(reg);
    _pWire->write(data, len);
    return (_pWire->endTransmission() == 0);
}

/**
 * @brief Initialize the IO Expander configuration and output states
 * 
 * This function configures all pins as outputs and clears the output registers.
 * 
 * @return true if initialization succeeds
 *           false if any I2C transmission fails
 */
bool IOExpander::begin()
{
    _pWire->begin();

    uint8_t cfg[IOE_PORT_BYTES] = {CONFIG_OUTPUT_MODE, CONFIG_OUTPUT_MODE, CONFIG_OUTPUT_MODE};
    // Configure all 24 IOs as outputs (default after power-on is inputs)
    if (!writeBytes(IOE_REG_CONFIG, cfg, IOE_PORT_BYTES))
    {
        return false;
    }

    _outputs[0] = REGISTER_OFF;
    _outputs[1] = REGISTER_OFF;
    _outputs[2] = REGISTER_OFF;
    if (!writeBytes(IOE_REG_OUTPUT, _outputs, IOE_PORT_BYTES))
    {
        return false;
    }

    return true;
}

    /**
     * @brief Enable or disable an eFuse output
     * @param channel Output index (0–23).Channels 0–7 correspond to port P0, 8–15 to port P1, and 16–23 to port P2.
     * @param outputState true → set HIGH (ON), false → set LOW (OFF)
     */
void IOExpander::setEfuse(uint8_t channel, bool outputState)
{
    if (channel >= OUTPUT_COUNT)
    {
        return;
    }

    // 0..7 -> port 0 (P0), 8..15 -> port 1 (P1), 16..23 -> port 2 (P2)
    uint8_t port = channel / 8;
    uint8_t bit  = channel % 8;

    if (outputState)
    {
        _outputs[port] |= static_cast<uint8_t>(1U << bit);   // ON -> bit = 1
    }
    else
    {
        _outputs[port] &= static_cast<uint8_t>(~(1U << bit)); // OFF -> bit = 0
    }

    writeBytes(IOE_REG_OUTPUT, _outputs, IOE_PORT_BYTES);
}
