#include "utils.h"
#include "bioreactor_controller.h"

void serialReader()
{
    if (Serial.available())
    {
        uint8_t byte = Serial.read();

        if (byte == 'a')
        {
            // Do something
        }
        else if (byte == 'b')
        {
            // Do something
        }
    }
}
