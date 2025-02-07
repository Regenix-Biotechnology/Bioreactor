#include "Example.h"

/**
 * @brief Constructor to initialize the Example class
 */
Example::Example() {}

/**
 * @brief Get the first value.
 * @return The stored value1.
 */
float Example::getValue1() const
{
    return value1;
}

/**
 * @brief Get the second value.
 * @return The stored value2.
 */
float Example::getValue2() const
{
    return value2;
}

/**
 * @brief Get the flag state.
 * @return True if the flag is set, false otherwise.
 */
bool Example::getFlag() const
{
    return flag;
}

/**
 * @brief Set the first value.
 * @param value The value to store in value1.
 */
void Example::setValue1(float value)
{
    value1 = value;
}

/**
 * @brief Set the second value.
 * @param value The value to store in value2.
 */
void Example::setValue2(float value)
{
    value2 = value;
}

/**
 * @brief Set the flag state.
 * @param flag The boolean value to set.
 */
void Example::setFlag(bool flag)
{
    this->flag = flag;
}
