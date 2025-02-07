#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <Arduino.h>

/**
 * @class Example
 * @brief A generic class template for future development.
 *
 * This class provides a basic structure with private member variables and
 * corresponding getter and setter methods.
 */
class Example
{
public:
    Example();

    // Getters
    float getValue1() const;
    float getValue2() const;
    bool getFlag() const;

    // Setters
    void setValue1(float value);
    void setValue2(float value);
    void setFlag(bool flag);

private:
    float value1;
    float value2;
    bool flag;
};

#endif
