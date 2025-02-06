#include <Arduino.h>
#include "SHT40.h"
#include "Pyroscience.h"

SHT40 sht40;
Pyroscience pyroscience;

void setup()
{
    Serial.begin(9600);
    Serial.println("Hello, World!");
}

void loop() {}
